/**
 * REXUS PIOneERS - Pi_1
 * raspi1.cpp
 * Purpose: Main logic control of the REXUS PIOneERS experiment handling data
 * communication and transfer as well as the boom and antenna sub-systems.
 *
 * @author David Amison
 * @version 2.2 12/10/2017
 */

#include <stdio.h>
#include <cstdint>
#include <unistd.h> //For sleep
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <signal.h>  // For catching signals!

#include "RPi_IMU/RPi_IMU.h"
#include "camera/camera.h"
#include "UART/UART.h"
#include "Ethernet/Ethernet.h"
#include "comms/pipes.h"
#include "comms/protocol.h"
#include "comms/packet.h"
#include "tests/tests.h"

#include <wiringPi.h>
#include "pins1.h"
#include "timing/timer.h"
#include "logger/logger.h"

#include <error.h>
#include <string.h>

Logger Log("/Docs/Logs/raspi1");

// Main inputs for experiment control
bool flight_mode = false;

// Motor Setup
int encoder_count = 0;
int encoder_rate = 100;

/**
 * Advances the encoder_count variable by one.
 */
void interrupt() {
	piLock(1);
	encoder_count++;
	piUnlock(1);
}

/**
 * Checks whether input is activated
 * @param pin: GPIO to be checked
 * @return true or false
 */
bool poll_input(int pin) {
	int count = 0;
	for (int i = 0; i < 5; i++) {
		count += digitalRead(pin);
		delayMicroseconds(200);
	}
	return (count < 3) ? true : false;
}

/**
 *  Checks the status of three input pins (in1, in2 and in3). If in1 and in2 are high
 *  but in3 is low return value will look like: 0b0000 0011
 *  @return Integer where the three LSB represent the status of in1, in2 and in3.
 */
int poll_signals(int in1, int in2, int in3) {
	int rtn = 0;
	if (poll_input(in1))
		rtn += 0b001;
	if (poll_input(in2))
		rtn += 0b010;
	if (poll_input(in3))
		rtn += 0b100;
	return rtn;
}


// Global variable for the Camera and IMU
PiCamera Cam;
RPi_IMU IMU; //  Not initialised yet to prevent damage during lift off
comms::Pipe IMU_stream;

// Setup for the UART communications
int baud = 38400; // TODO find right value for RXSM
RXSM REXUS(baud);
comms::Pipe rxsm_stream;

// Ethernet communication setup and variables (we are acting as client)
int port_no = 31415; // Random unused port for communication
std::string server_name = "169.254.86.24";
Raspi1 raspi1(port_no, server_name);

/**
 * Handles any SIGINT signals received by the program (i.e. ctrl^c), making sure
 * we end all child processes cleanly and reset the gpio pins.
 * @param s: Signal received
 */
void signal_handler(int s) {
	Log("FATAL") << "Exiting program after signal " << s;
	REXUS.sendMsg("Ending Program");
	REXUS.end_buffer();
	// TODO send exit signal to Pi 2!
	Log("INFO") << "Stopping camera and IMU processes";
	IMU.stopDataCollection();
	Cam.stopVideo();
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	
	Log("INFO") << "Ending program, Pi rebooting";
	REXUS.sendMsg("Pi Rebooting");
	system("sudo reboot");
	exit(1); // This was an unexpected end so we will exit with an error!
}

/**
 * Checks the status of all possible child processes and returns as a
 * string.
 */
 std::string status_check() {
	std::string rtn;
	if (raspi1.status())
		rtn += "Eth_u, ";
	else
		rtn += "Eth_d, ";

	if (REXUS.status())
		rtn += "RXSM_u, ";
	else
		rtn += "RXSM_d, ";

	if (Cam.status())
		rtn += "Cam_u, ";
	else
		rtn += "Cam_d, ";
	
	if (IMU.status())
		rtn += "IMU_u";
	else
		rtn += "IMU_d";
	return rtn;
 }

/**
 * When the 'Start of Data Storage' signal is received all data recording
 * is stopped (IMU and Camera) and power to the camera is cut off to stop
 * shorting due to melting on re-entry. All data is copied into a backup
 * directory.
 * @return 0 for success, otherwise for failure
 */
int SODS_SIGNAL() {
	Log("INFO") << "SODS signal received";
	std::cout << "SODS received" << std::endl;
	REXUS.sendMsg("SODS received");
	if (Cam.status()) {
		Log("INFO") << "Camera still running";
		REXUS.sendMsg("Camera still running :D");
	} else {
		Log("ERROR") << "Camera process died prematurely or did not start";
		Log("INFO") << "Trying to restart camera";
		REXUS.sendMsg("Camera died, attempting restart");
		Cam.startVideo("Docs/Video/rexus_video");
	}
	Log("INFO") << "Ending IMU process";
	IMU.stopDataCollection();
	//To make sure motor isn't turning
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// TODO copy data to a further backup directory
	Log("INFO") << "Waiting for power off";
	comms::Packet p1;
	while(1) {
		REXUS.sendMsg("I'm falling...");
		while (raspi1.recvPacket(p1))
			REXUS.sendPacket(p1);
		Timer::sleep_ms(5000);
	}
	return 0;
}

/**
 * When the 'Start of Experiment' signal is received the boom needs to be
 * deployed and the ImP and IMU to start taking measurements. For boom
 * deployment is there is no increase in the encoder count or ?? seconds
 * have passed since start of deployment then it is assumed that either the
 * boom has reached it's full length or something has gone wrong and the
 * count of the encoder is sent to ground.
 * @return 0 for success, otherwise  for failure
 */
int SOE_SIGNAL() {
	Log("INFO") << "SOE signal received";
	REXUS.sendMsg("SOE received");
	// Setup the IMU and start recording
	// TODO ensure IMU setup register values are as desired
	IMU.setupAcc();
	IMU.setupGyr();
	IMU.setupMag();
	Log("INFO") << "IMU setup";
	// Start data collection and store the stream where data is coming through
	IMU_stream = IMU.startDataCollection("Docs/Data/Pi1/imu_data");
	Log("INFO") << "IMU collecting data";
	comms::Packet p;
	if (flight_mode) {
		piLock(1);
		encoder_count = 0;
		piUnlock(1);
		REXUS.sendMsg("Extending boom");
		// Extend the boom!
		int count = encoder_count;
		int diff = encoder_rate;
		Timer tmr;
		digitalWrite(MOTOR_CW, 1);
		digitalWrite(MOTOR_ACW, 0);
		Log("INFO") << "Motor triggered, boom deploying";
		Log("INFO") << "Starting Loop";
		// Keep checking the encoder count till it reaches the required amount.
		int counter = 0;
		std::stringstream strs;
		while (count < 19500) {
			// Lock is used to keep everything thread safe
			piLock(1);
			diff = encoder_count - count;
			count = encoder_count;
			piUnlock(1);
			Log("INFO") << "Encoder count- " << encoder_count;
			Log("INFO") << "Encoder rate- " << diff * 10 << " counts/sec";
			// Occasionally send count to ground
			if (counter++ >= 100) {
				counter = 0;
				strs << "Count: " << encoder_count << " Rate: " <<
						diff * 10;
				REXUS.sendMsg(strs.str());
				// Empty the stringstream
				strs.str("");
				strs.clear();
			}
			// Check the boom is actually deploying (give it at least 30 secods to deploy)
			if ((tmr.elapsed() > 30000) && (diff < 10)) {
				Log("ERROR") << "Boom not deploying as expected";
				break;
			}
			// Read data from IMU_data_stream and echo it to Ethernet and RXSM
			int n = IMU_stream.binread(&p, sizeof (comms::Packet));
			if (n > 0) {
				Log("DATA (IMU1)") << p;
				REXUS.sendPacket(p);
				raspi1.sendPacket(p);
				Log("INFO") << "Data sent to Pi2 and RXSM";
			}
			n = raspi1.recvPacket(p);
			if (n > 0) {
				Log("DATA (PI2)") << p;
				REXUS.sendPacket(p);
				Log("INFO") << "Data echod to RXSM";
			}
			// TODO what about when there is an error (n < 0)
			delay(10);
		}
		digitalWrite(MOTOR_CW, 0); // Stops the motor.
		Log("INFO") << "Boom extended by " << count;
		std::stringstream ss;
		ss << "Boom extended by " << count;
		REXUS.sendMsg(ss.str());
	}
	Log("INFO") << "Waiting for SODS";
	// Wait for the next signal to continue the program
	bool signal_received = false;
	int counter = 0;
	while (!signal_received) {
		if (counter++ >= 100) {
			// Check the camera and imu are still running
			counter = 0;
			// Send general status update
			REXUS.sendMsg(status_check());
			if (!Cam.status()) {
				Log("ERROR") << "Camera stopped running...restarting";
				Cam.startVideo("Docs/Video/restart");
			}
			if (!IMU.status()) {
				Log("ERROR") << "IMU stopped running...restarting";
				IMU.startDataCollection("Docs/Data/Pi1/restart");
			}

		}
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = (poll_signals(LO, SOE, SODS) & 0b100);
		// Read data from IMU_data_stream and echo it to Ethernet
		int n = IMU_stream.binread(&p, sizeof (comms::Packet));
		if (n > 0) {
			Log("DATA (IMU1)") << p;
			REXUS.sendPacket(p);
			raspi1.sendPacket(p);
			Log("INFO") << "Data sent to Ethernet Communications";
		}
		n = raspi1.recvPacket(p);
		if (n > 0) {
			Log("DATA (PI2)") << p;
			REXUS.sendPacket(p);
		}
		delay(10);
	}
	return SODS_SIGNAL();
}

/**
 * When the 'Lift Off' signal is received from the REXUS Module the cameras
 * are set to start recording video and we then wait to receive the 'Start
 * of Experiment' signal (when the nose-cone is ejected)
 */
int LO_SIGNAL() {
	Log("INFO") << "LO signal received";
	REXUS.sendMsg("LO received");
	Cam.startVideo("Docs/Video/rexus_video");
	Log("INFO") << "Camera recording";
	REXUS.sendMsg("Recording Video");
	// Poll the SOE pin until signal is received
	Log("INFO") << "Waiting for SOE";
	REXUS.sendMsg("Waiting for SOE");
	bool signal_received = false;
	int counter = 0;
	comms::Packet p;
	while (!signal_received) {
		delay(10);
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = (poll_signals(LO, SOE, SODS) & 0b110);
		// Send a message every second for the sake of sanity!
		if (counter++ >= 100) {
			counter = 0;
			REXUS.sendMsg("I'm still alive...");
			REXUS.sendMsg(status_check());
			// Specifically check if the camera is still running
			if (!Cam.status()) {
				Log("ERROR") << "Camera not running...restarting";
				Cam.startVideo("Docs/Video/restart");
			}
		}
		// Check for packets from pi2
		while (raspi1.recvPacket(p))
			REXUS.sendPacket(p);
	}
	return SOE_SIGNAL();
}

/**
 * This part of the program is run before the Lift-Off. In effect it
 * continually listens for commands from the ground station and runs any
 * required tests, regularly reporting status until the LO Signal is
 * received.
 */
int main(int argc, char* argv[]) {
	// Create necessary directories for saving files
	signal(SIGINT, signal_handler);
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video Docs/Logs");
	Log.start_log();
	REXUS.buffer();
	Log("INFO") << "Pi 1 is running";
	REXUS.sendMsg("Pi 1 Alive");
	// Setup wiringpi
	wiringPiSetup();
	// Setup main signal pins
	pinMode(LO, INPUT);
	pullUpDnControl(LO, PUD_UP);
	pinMode(SOE, INPUT);
	pullUpDnControl(SOE, PUD_UP);
	pinMode(SODS, INPUT);
	pullUpDnControl(SODS, PUD_UP);
	pinMode(ALIVE, INPUT);
	pullUpDnControl(ALIVE, PUD_DOWN);
	Log("INFO") << "Main signal pins setup" << std::endl;

	// Setup pins and check whether we are in flight mode
	pinMode(LAUNCH_MODE, INPUT);
	pullUpDnControl(LAUNCH_MODE, PUD_UP);
	flight_mode = digitalRead(LAUNCH_MODE);
	Log("INFO") << (flight_mode ? "flight mode enabled" : "test mode enabled");
	if (flight_mode)
		REXUS.sendMsg("WARNING: Flight mode enabled");
	else
		REXUS.sendMsg("Entering test mode");
	// Setup Motor Pins
	pinMode(MOTOR_CW, OUTPUT);
	pinMode(MOTOR_ACW, OUTPUT);
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	wiringPiISR(MOTOR_IN, INT_EDGE_RISING, interrupt);
	Log("INFO") << "Pins for motor control setup";
	// Set all IMU sensorts to off
	IMU.resetRegisters();
	Log("INFO") << "Powered off all IMU sensors";
	// Wait for GPIO to go high signalling that Pi2 is ready to communicate
	Timer tmr;
	while (tmr.elapsed() < 20000) {
		if (digitalRead(ALIVE)) {
			Log("INFO") << "Establishing ethernet connection";
			raspi1.run("Docs/Data/Pi2/backup");
			if (raspi1.status()) {
				Log("INFO") << "Connection successful";
				REXUS.sendMsg("Ethernet connection successful");
				break;
			} else {
				Log("INFO") << "Connection failed";
				REXUS.sendMsg("ERROR: Ethernet connection failed");
				REXUS.sendMsg("Continuing wihout ethernet comms");
				break;
			}
			break;
		}
		Timer::sleep_ms(10);
	}
	if (tmr.elapsed() >= 20000) {
		REXUS.sendMsg("ERROR: Timeout waiting for Pi 2");
		Log("ERROR") << "Timeout waiting for Pi 2";
		Log("INFO") << "Attempting ethernet connection anyway";
		raspi1.run("Docs/Data/Pi2/backup");
		Timer::sleep_ms(5000);
		if (raspi1.status()) {
			Log("INFO") << "Ethernet connection successful";
			REXUS.sendMsg("Ethernet connected");
		} else {
			Log("ERROR") << "Unable to establish ethernet connection";
			REXUS.sendMsg("ERROR: Ethernet failed");
			REXUS.sendMsg("Continuing without ethernet comms");
		}
	}
	Log("INFO") << "Waiting for LO";
	REXUS.sendMsg("Waiting for LO");
	// Wait for LO signal
	bool signal_received = false;
	comms::Packet p;
	comms::byte1_t id;
	comms::byte2_t index;
	comms::byte1_t data[16];
	int n;
	int counter;
	while (!signal_received) {
		Timer::sleep_ms(10);
		// Implements a loop to ensure LO signal has actually been received
		signal_received = (poll_signals(LO, SOE, SODS) & 0b111);
		if (counter ++ > 1000) {
			REXUS.sendMsg("Ping");
			counter = 0;
		}
		//Check for packets from Pi2
		n = raspi1.recvPacket(p);
		if (n > 0)
			REXUS.sendPacket(p);
		// Check for any packets from RXSM
		n = REXUS.recvPacket(p);
		if (n > 0) {
			REXUS.sendMsg("ACK");
			Log("RXSM") << p;
			raspi1.sendPacket(p);
			comms::Protocol::unpack(p, id, index, data);
			if (id == 0b11000000) {
				switch (data[0]) {
					case 1: // restart
					{
						Log("INFO") << "Rebooting...";
						system("sudo reboot now");
						break;
					}
					case 2: // shutdown
					{
						Log("INFO") << "Shutting down...";
						system("sudo shutdown now");
						break;
					}
					case 3: // Change between flight and test mode
					{
						flight_mode = data[1];
						Log("INFO") << (flight_mode ? "flight mode enabled" : "test mode enabled");
						if (flight_mode)
							REXUS.sendMsg("WARNING: Flight mode enabled");
						else
							REXUS.sendMsg("Entering test mode");
						break;
					}
					case 4: // Run all tests
					{
						Log("INFO") << "Running Tests";
						std::string result = tests::pi1_tests();
						REXUS.sendMsg(result);
						Log("INFO") << "Test Results\n\t" << result;
						break;
					}
					case 5:
					{
						Log("INFO") << "Cleaning files";
						if (data[1] == 0) {
							//Clean everything
							system("sudo rm -rf /Docs/Data/Pi1/*.txt");
							system("sudo rm -rf /Docs/Data/Pi2/*.txt");
							system("sudo rm -rf /Docs/Video/*.h264");
							system("sudo rm -rf /Docs/Data/Logs/*.txt");
						} else if (data[1] == 1) {
							//Clean data
							system("sudo rm -rf /Docs/Data/Pi1/*.txt");
							system("sudo rm -rf /Docs/Data/Pi2/*.txt");
						} else if (data[1] == 2) {
							//Clean video
							system("sudo rm -rf /Docs/Video/*.h264");
						} else if (data[1] == 3) {
							//Clean logs
							system("sudo rm -rf /Docs/Data/Logs/*.txt");
						}
						Timer::sleep_ms(5000);
						REXUS.sendMsg("Files cleaned... rebooting");
						system("sudo reboot");
						break;
					}
					case 6:
					{
						Log("INFO") << "Rebuilding software";
						system("sudo rm -rf /home/pi/CPP_PIOneERS/bin/raspi1");
						system("sudo rm -rf /home/pi/CPP_PIOneERS/build/*.o");
						system("sudo make ./bin/raspi1 -C /home/pi/CPP_PIOneERS");
						Timer::sleep_ms(20000);
						REXUS.sendMsg("Project rebuilt... rebooting");
						system("sudo reboot");
						break;
					}
					default:
					{
						REXUS.sendMsg("Not Recognised");
						Log("ERROR") << "Command not recognised" << data[0];
						break;
					}
				}
			}
		}
	}
	LO_SIGNAL();
	return 0;
}

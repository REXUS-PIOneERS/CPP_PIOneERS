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
std::string server_name = "raspi2.local";
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
	if (Cam.is_running()) {
		Cam.stopVideo();
		Log("INFO") << "Stopping camera process";
	} else {
		Log("ERROR") << "Camera process died prematurely or did not start";
	}

	if (raspi1.is_alive()) {
		raspi1.end();
		Log("INFO") << "Closed Ethernet communication";
	} else {
		Log("ERROR") << "Ethernet process died prematurely or did not start";
	}
	if (&IMU_stream != NULL) {
		IMU_stream.close_pipes();
		Log("INFO") << "Closed IMU communication";
	} else {
		Log("ERROR") << "IMU process died prematurely or did not start";
	}
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// TODO copy data to a further backup directory
	Log("INFO") << "Ending program, Pi rebooting";
	REXUS.sendMsg("Pi Rebooting");
	system("sudo reboot");
	exit(1); // This was an unexpected end so we will exit with an error!
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
	/*
	if (Cam.is_running()) {
		Cam.stopVideo();
		Log("INFO") << "Stopping camera process";
	} else {
		Log("ERROR") << "Camera process died prematurely or did not start";
	}
	*/
	if (raspi1.is_alive()) {
		raspi1.end();
		Log("INFO") << "Closed Ethernet communication";
	} else {
		Log("ERROR") << "Ethernet process died prematurely or did not start";
	}
	if (&IMU_stream != NULL) {
		IMU_stream.close_pipes();
		Log("INFO") << "Closed IMU communication";
	} else {
		Log("ERROR") << "IMU process died prematurely or did not start";
	}
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// TODO copy data to a further backup directory
	Log("INFO") << "Waiting for power off";
	while(1) {
		REXUS.sendMsg("I'm falling...");
		Timer::sleep_ms(5000);
	}
	//system("sudo reboot");
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
	std::cout << "SOE received" << std::endl;
	REXUS.sendMsg("SOE received");
	// Setup the IMU and start recording
	// TODO ensure IMU setup register values are as desired
	IMU.setupAcc();
	IMU.setupGyr();
	IMU.setupMag();
	Log("INFO") << "IMU setup";
	// Start data collection and store the stream where data is coming through
	IMU_stream = IMU.startDataCollection("Docs/Data/Pi1/test");
	Log("INFO") << "IMU collecting data";
	//comms::byte1_t buf[20]; // Buffer for storing data
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
		int i = 0;
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
			if (tmr.elapsed() > (1000 * i)) {
				i++;
				strs << "Count: " << encoder_count << " Rate: " <<
						diff * 10;
				REXUS.sendMsg(strs.str());
				strs.clear();
			}
			// Check the boom is actually deploying
			if ((tmr.elapsed() > 20000) && (diff < 10)) {
				Log("ERROR") << "Boom not deploying as expected";
				break;
			}
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
			// TODO what about when there is an error (n < 0)

			delay(100);
		}
		digitalWrite(MOTOR_CW, 0); // Stops the motor.
		double dist = 0.0833 * (count / 1000);
		Log("INFO") << "Boom deployed to approx: " << dist << " m";
		std::stringstream ss;
		ss << "Boom deployed to " << dist << " m";
		REXUS.sendMsg(ss.str());
	}
	Log("INFO") << "Waiting for SODS";
	// Wait for the next signal to continue the program
	bool signal_received = false;
	while (!signal_received) {
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = poll_input(SODS);
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
	std::cout << "LO Recevied" << std::endl;
	Log("INFO") << "LO signal received";
	REXUS.sendMsg("LO received");
	Cam.startVideo("Docs/Video/rexus_video");
	Log("INFO") << "Camera recording";
	REXUS.sendMsg("Recording Video");
	// Poll the SOE pin until signal is received
	// TODO implement check to make sure no false signals!
	Log("INFO") << "Waiting for SOE";
	REXUS.sendMsg("Waiting for SOE");
	bool signal_received = false;
	int counter = 0;
	while (!signal_received) {
		delay(10);
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = poll_input(SOE);
		// Send a message every second for the sake of sanity!
		if (counter >= 100) {
			counter = 0;
			REXUS.sendMsg("I'm still alive...");
		} else
			counter++;

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
	// Wait for GPIO to go high signalling that Pi2 is ready to communicate
	Timer tmr;
	while (tmr.elapsed() < 20000) {
		if (digitalRead(ALIVE)) {
			Log("INFO") << "Establishing ethernet connection";
			raspi1.run("Docs/Data/Pi2/backup");
			//if (raspi1.is_alive()) {
			//	Log("INFO") << "Connection successful";
			//	REXUS.sendMsg("Ethernet connection successful");
			//	break;
			//} else {
			//	Log("INFO") << "Connection failed";
			//	REXUS.sendMsg("ERROR: Ethernet connection failed");
			//	break;
			//}
			break;
		}
	}
	if (tmr.elapsed() > 20000) {
		REXUS.sendMsg("ERROR: Timeout waiting for Pi 2");
		Log("ERROR") << "Timeout waiting for Pi 2";
		Log("INFO") << "Attempting ethernet connection anyway";
		raspi1.run("Docs/Data/Pi2/backup");
		if (raspi1.is_alive()) {
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
	while (!signal_received) {
		Timer::sleep_ms(10);
		// Implements a loop to ensure LO signal has actually been received
		signal_received = poll_input(LO);
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
			Log("UNPACKED") << "ID: " << id << "DATA[0]: " << data[0];
			if (id == 0b11000000) {
				switch (data[0]) {
					case 1: // restart
					{
						Log("INFO") << "Rebooting...";
						system("sudo reboot now");
						exit(0);
					}
					case 2: // shutdown
					{
						Log("INFO") << "Shutting down...";
						system("sudo shutdown now");
						exit(0);
					}
					case 3: // Toggle flight mode
					{
						Log("INFO") << "Toggling flight mode";
						flight_mode = (flight_mode) ? false : true;
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

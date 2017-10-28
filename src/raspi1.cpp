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
#include <signal.h>  // For catching signals!

#include "RPi_IMU/RPi_IMU.h"
#include "camera/camera.h"
#include "UART/UART.h"
#include "Ethernet/Ethernet.h"
#include "comms/pipes.h"
#include "comms/protocol.h"
#include "comms/packet.h"

#include <wiringPi.h>

// Main inputs for experiment control
int LO = 29;
int SOE = 28;
int SODS = 27;

int LAUNCH_MODE_OUT = 10;
int LAUNCH_MODE_IN = 11;
bool flight_mode = false;

// Motor Setup
int MOTOR_CW = 25;
int MOTOR_ACW = 24;
int MOTOR_IN = 0;
int encoder_count = 0;

/**
 * Advances the encoder_count variable by one.
 */
void count_encoder() {
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
PiCamera Cam = PiCamera();
RPi_IMU IMU; //  Not initialised yet to prevent damage during lift off
comms::Pipe IMU_stream;

// Setup for the UART communications
int baud = 230400; // TODO find right value for RXSM
UART RXSM = UART();
comms::Pipe UART_stream;

// Ethernet communication setup and variables (we are acting as client)
int port_no = 31415; // Random unused port for communication
std::string server_name = "raspi2.local";
comms::Pipe ethernet_stream; // 0 = read, 1 = write
Client raspi1 = Client(port_no, server_name);
int ALIVE = 3;

/**
 * Handles any SIGINT signals received by the program (i.e. ctrl^c), making sure
 * we end all child processes cleanly and reset the gpio pins.
 * @param s: Signal received
 */
void signal_handler(int s) {
	std::cout << "Exiting program..." << std::endl;
	// TODO send exit signal to Pi 2!
	Cam.stopVideo();
	if (&ethernet_stream != NULL)
		ethernet_stream.close_pipes();
	if (&IMU_stream != NULL)
		IMU_stream.close_pipes();

	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	digitalWrite(LAUNCH_MODE_OUT, 0);
	//system("sudo shutdown now");
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
	std::cout << "SIGNAL: SODS" << std::endl;
	Cam.stopVideo();
	if (&ethernet_stream != NULL)
		ethernet_stream.close_pipes();
	if (&IMU_stream != NULL)
		IMU_stream.close_pipes();
	std::cout << "Ending program!" << std::endl;
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// TODO copy data to a further backup directory
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
	std::cout << "SIGNAL: SOE" << std::endl;
	// Setup the IMU and start recording
	// TODO ensure IMU setup register values are as desired
	IMU = RPi_IMU();
	IMU.setupAcc();
	IMU.setupGyr();
	IMU.setupMag();
	// Start data collection and store the stream where data is coming through
	IMU_stream = IMU.startDataCollection("Docs/Data/Pi1/test");
	//comms::byte1_t buf[20]; // Buffer for storing data
	comms::Packet p;
	// Extend the boom!
	wiringPiISR(MOTOR_IN, INT_EDGE_RISING, count_encoder);
	digitalWrite(MOTOR_CW, 1);
	digitalWrite(MOTOR_ACW, 0);
	// Keep checking the encoder count till it reaches the required amount.
	std::cout << "INFO: Boom deploying..." << std::endl;
	while (1) {
		// Lock is used to keep everything thread safe
		piLock(1);
		if (encoder_count >= 10000) // TODO what should the count be?
			break;
		std::cout << " " << encoder_count << "..." << std::endl;
		piUnlock(1);
		// Read data from IMU_data_stream and echo it to Ethernet
		int n = IMU_stream.binread(&p, sizeof (p));
		if (n > 0) {
			std::cout << "IMU1: " << p << std::endl; // TODO change to send to RXSM
			ethernet_stream.binwrite(&p, sizeof (p));
		}
		n = ethernet_stream.binread(&p, sizeof (p));
		if (n > 0)
			std::cout << "PI2: " << p << std::endl;
		delay(100);
	}
	digitalWrite(MOTOR_CW, 0); // Stops the motor.
	std::cout << "INFO: Boon deployed" << std::endl;

	// Wait for the next signal to continue the program
	bool signal_received = false;
	while (!signal_received) {
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = poll_input(SODS);
		// Read data from IMU_data_stream and echo it to Ethernet
		int n = IMU_stream.binread(&p, sizeof (p));
		if (n > 0) {
			std::cout << "IMU1: " << p << std::endl; // TODO change to send to RXSM
			ethernet_stream.binwrite(&p, sizeof (p));
		}
		n = ethernet_stream.binread(&p, sizeof (p));
		if (n > 0)
			std::cout << "PI2: " << p << std::endl;
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
	std::cout << "SIGNAL: LO" << std::endl;
	Cam.startVideo("Docs/Video/rexus_video");
	// Poll the SOE pin until signal is received
	// TODO implement check to make sure no false signals!
	std::cout << "Waiting for SOE.." << std::endl;
	bool signal_received = false;
	while (!signal_received) {
		delay(10);
		// Implements a loop to ensure SOE signal has actually been received
		signal_received = poll_input(SOE);
		// TODO Implement communications with RXSM
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
	signal(SIGINT, signal_handler);
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

	// Setup pins and check whether we are in flight mode
	pinMode(LAUNCH_MODE_OUT, OUTPUT);
	pinMode(LAUNCH_MODE_IN, INPUT);
	pullUpDnControl(LAUNCH_MODE_IN, PUD_DOWN);
	digitalWrite(LAUNCH_MODE_OUT, 1);
	flight_mode = digitalRead(LAUNCH_MODE_IN);

	// Setup Motor Pins
	pinMode(MOTOR_CW, OUTPUT);
	pinMode(MOTOR_ACW, OUTPUT);
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// Create necessary directories for saving files
	std::cout << "Pi 1 is running..." << std::endl;
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video");
	// Wait for GPIO to go high signalling that Pi2 is ready to communicate
	while (!digitalRead(ALIVE)) {
		delay(10);
		std::cout << "Pi 2 is on, trying to establish Ethernet connection..." << std::endl;
		// Try to connect to Pi 2
		ethernet_stream = raspi1.run("Docs/Data/Pi2/backup.txt");
		// TODO handle error where we can't connect to the server
		std::cout << "Connection successful" << std::endl << "Waiting for LO..." <<
				std::endl;
		// Wait for LO signal
		bool signal_received = false;
		while (!signal_received) {
			delay(10);
			// Implements a loop to ensure LO signal has actually been received
			signal_received = poll_input(LO);
			// TODO Implement communications with RXSM
		}
		LO_SIGNAL();
		//system("sudo reboot");
		return 0;
	}

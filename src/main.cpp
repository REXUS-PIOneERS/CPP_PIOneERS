/*
 * Pi 1 is connected to the ImP/IMU via UART, Camera via CSI, Burn Wire Relay
 * via GPIO and Pi 1 via Ethernet and GPIO for LO, SOE and SODS signals.
 *
 * This program controls most of the main logic and for the PIOneERs mission
 * on board REXUS.
 */

#include <stdio.h>
#include <cstdint>
#include <unistd.h> //For sleep
#include <stdlib.h>
#include <iostream>

#include "RPi_IMU/RPi_IMU.h"
#include "camera/camera.h"
#include "UART/UART.h"
#include "Ethernet/Ethernet.h"
#include "pipes/pipes.h"

#include <wiringPi.h>

// Main inputs for experiment control
int LO = 29;
int SOE = 28;
int SODS = 27;

// Burn Wire Setup
int MOTOR_PWM = 1;
int MOTOR_CW = 4;
int MOTOR_ACW = 5;
int MOTOR_IN = 0;
int PWM_RANGE = 2000;
int PWM_CLOCK = 2000;
int encoder_count = 0;

void count_encoder() {
	// Advances the encoder_count variable by one.
	piLock(1);
	encoder_count++;
	piUnlock(1);
}

// Global variable for the Camera and IMU
PiCamera Cam = PiCamera();
RPi_IMU IMU; //  Not initialised yet to prevent damage during lift off
Pipe IMU_stream;

// Setup for the UART communications
int baud = 230400;
UART RXSM = UART();
Pipe UART_stream;

// Ethernet communication setup and variables (we are acting as client)
int port_no = 31415; // Random unused port for communication
std::string server_name = "PIOneERS1.local";
Pipe ethernet_stream; // 0 = read, 1 = write
Client ethernet_comms = Client(port_no, server_name);
int ALIVE = 2;

int SODS_SIGNAL() {
	/*
	 * When the 'Start of Data Storage' signal is received all data recording
	 * is stopped (IMU and Camera) and power to the camera is cut off to stop
	 * shorting due to melting on re-entry. All data is copied into a backup
	 * directory.
	 */
	fprintf(stdout, "Signal Received: SODS\n");
	fflush(stdout);
	Cam.stopVideo();
	IMU_stream.close_pipes();
	//IMU.stopDataCollection();
	// Send terminate message to server
	ethernet_stream.strwrite("EXIT");
	ethernet_stream.close_pipes();
	// TODO copy data to a further backup directory
	return 0;
}

int SOE_SIGNAL() {
	/*
	 * When the 'Start of Experiment' signal is received the boom needs to be
	 * deployed and the ImP and IMU to start taking measurements. For boom
	 * deployment is there is no increase in the encoder count or ?? seconds
	 * have passed since start of deployment then it is assumed that either the
	 * boom has reached it's full length or something has gone wrong and the
	 * count of the encoder is sent to ground.
	 */
	fprintf(stdout, "Signal Received: SOE\n");
	// Setup the IMU and start recording
	// TODO ensure IMU setup register values are as desired
	IMU = RPi_IMU();
	IMU.setupAcc();
	IMU.setupGyr();
	IMU.setupMag();
	// Start data collection and store the stream where data is coming through
	IMU_stream = IMU.startDataCollection("Docs/Data/Pi1/test");
	char buf[256]; // Buffer for reading data from the IMU stream
	// Extend the boom!
	wiringPiISR(MOTOR_IN, INT_EDGE_RISING, count_encoder);
	digitalWrite(MOTOR_CW, 1);
	digitalWrite(MOTOR_ACW, 0);
	// Keep checking the encoder count till it reaches the required amount.
	fprintf(stdout, "INFO: Boom deploying...\n");
	while (1) {
		// Lock is used to keep everything thread safe
		piLock(1);
		if (encoder_count >= 10000) // TODO what should the count be?
			break;
		fprintf(stdout, "INFO: Encoder Count-%d\n", encoder_count);
		piUnlock(1);
		// TODO periodically send the count to ground
		int n = IMU_stream.binread(buf, 255);
		if (n > 0) {
			buf[n] = '\0';
			//fprintf(stdout, "DATA (%d): %s\n", n, buf); // TODO change to send to RXSM
			ethernet_stream.binwrite(buf, n);
		}
		delay(100);
	}
	digitalWrite(MOTOR_CW, 0); // Stops the motor.
	fprintf(stdout, "INFO: Boom deployed :D\n");
	fflush(stdout);

	// Wait for the next signal to continue the program
	while (digitalRead(SODS)) {
		// Read data from IMU_data_stream and echo it to Ethernet
		char buf[256];
		int n = IMU_stream.binread(buf, 255);
		if (n > 0) {
			buf[n] = '\0';
			//fprintf(stdout, "DATA: %s\n", buf); // TODO change to send to RXSM
			ethernet_stream.binwrite(buf, n);
		}
		delay(100);
	}
	return SODS_SIGNAL();
}

int LO_SIGNAL() {
	/*
	 * When the 'Lift Off' signal is received from the REXUS Module the cameras
	 * are set to start recording video and we then wait to receive the 'Start
	 * of Experiment' signal (when the nose-cone is ejected)
	 */
	fprintf(stdout, "Signal Received: LO\n");
	Cam.startVideo();
	// Poll the SOE pin until signal is received
	// TODO implement check to make sure no false signals!
	fprintf(stdout, "Waiting for SOE signal...\n");
	while (digitalRead(SOE)) {
		// TODO implement RXSM communications
		delay(10);
	}
	return SOE_SIGNAL();
}

int main() {
	/*
	 * This part of the program is run before the Lift-Off. In effect it
	 * continually listens for commands from the ground station and runs any
	 * required tests, regularly reporting status until the LO Signal is
	 * received.
	 */
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

	// Setup Motor Pins
	pinMode(MOTOR_CW, OUTPUT);
	pinMode(MOTOR_ACW, OUTPUT);
	digitalWrite(MOTOR_CW, 0);
	digitalWrite(MOTOR_ACW, 0);
	// Create necessary directories for saving files
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video");
	fprintf(stdout, "Pi 1 is alive and running.\n");
	// Wait for GPIO to go high signalling that Pi2 is ready to communicate
	while (!digitalRead(ALIVE))
		delay(10);
	fprintf(stdout, "Pi 2 is now running, trying to establish Ethernet connection.\n");
	// Try to connect to Pi 2
	ethernet_stream = ethernet_comms.run()
			// TODO handle error where we can't connect to the server
			fprintf(stdout, "Connection to server successful\nWaiting for LO signal...\n");
	// Check for LO signal.
	bool signal_recieved = false;
	while (!signal_recieved) {
		delay(10);
		// Implements a loop to ensure LO signal has actually been received
		if (digitalRead(LO)) {
			int count = 0;
			for (int i = 0; i < 5; i++) {
				count += digitalRead(LO);
				delayMicroseconds(200);
			}
			if (count >= 3) signal_recieved = true;
		}
		// TODO Implement communications with RXSM
	}
	return LO_SIGNAL();
}


/*

Redundant code for this file but will be needed later for the data stream
to be read by the ethernet and by the UART.

for (int i = 0; i < 10; i++) {
	char buf[256];
			sleep(1);
			int num_char = read(data_stream, buf, 255);
	if (num_char < 0) {
		fprintf(stderr, "Error reading data from IMU stream\n");
	} else if (num_char == 0) {
		fprintf(stdout, "There was no data to read from the IMU stream\n");
	} else {
		buf[num_char] = '\0';
				fprintf(stdout, "DATA(%d): ", num_char / 2);
		for (int i = 0; i < num_char; i += 2) {
			std::int16_t datum = (buf[i] << 0 | buf[i + 1] << 8);
					fprintf(stdout, "%d ", datum);
		}
		fprintf(stdout, "\n");
	}
	fflush(stdout);
}
 */

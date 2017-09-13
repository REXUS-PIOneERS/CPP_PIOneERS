#include <stdio.h>
#include <cstdint>
#include <unistd.h> //For sleep
#include <stdlib.h>
#include <iostream>

#include "RPi_IMU.h"
#include "camera.h"
#include <wiringPi.h>

// Main inputs for experiment control
int LO = 29;
int SOE = 28;
int SODS = 27;

// Motor Setup (PWM Control)
int MOTOR_OUT = 1;
int MOTOR_IN = 0;
int PWM_RANGE = 2000;
int PWM_CLOCK = 2000;
int encoder_count = 0;

void count_encoder() {
	// Advances the encoder_count variable by one. Lock is used to keep
	// everything thread-safe.
	piLock(1);
	encoder_count++;
	piUnlock(1);
}

// Global variable for the Camera and IMU
PiCamera Cam = PiCamera();
RPi_IMU IMU; //  Not initialised yet to prevent damage during lift off
int IMU_data_stream;

// TODO Setup Ethernet communication variables

// TODO Setup UART connections

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
	IMU.stopDataCollection();
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
	fflush(stdout);
	IMU = RPi_IMU();
	IMU.setupAcc();
	IMU.setupGyr();
	IMU.setupMag();
	// Start data collection and store the stream where data is coming through
	IMU_data_stream = IMU.startDataCollection("Docs/Data/Pi1/test");
	// TODO Pass the data stream to UART so data can be sent to ground
	wiringPiISR(MOTOR_IN, INT_EDGE_RISING, count_encoder);
	pwmWrite(1, 1000); // Second number should be half of range set above
	// Keep checking the encoder count till it reaches the required amount.
	while (1) {
		// Lock is used to keep everything thread safe
		piLock(1);
		if (encoder_count >= 40) // TODO what should the count be?
			break;
		piUnlock(1);
		delay(500);
	}
	piUnlock(1);
	pwmWrite(1, 0); // Stops sending pulses through the PWM pin.
	fprintf(stdout, "Info: Boom deployed!\n");
	fflush(stdout);

	// Wait for the next signal to continue the program
	while (digitalRead(SODS))
		delay(10);
	return SODS_SIGNAL();
}

int LO_SIGNAL() {
	/*
	 * When the 'Lift Off' signal is received from the REXUS Module the cameras
	 * are set to start recording video and we then wait to receive the 'Start
	 * of Experiment' signal (when the nose-cone is ejected)
	 */
	fprintf(stdout, "Signal Received: LO\nWaiting 10 seconds\n");
	fflush(stdout);
	delay(10000); // Wait 10 seconds TODO check timing
	Cam.startVideo();
	fprintf(stdout, "Video running in background\n");
	fflush(stdout);
	// Poll the SOE pin until signal is received
	while (digitalRead(SOE))
		delay(10); // 10 s delay to save CPU
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

	// Setup PWM
	pinMode(MOTOR_OUT, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(PWM_RANGE);
	pwmSetClock(PWM_CLOCK); //Freq = 19200000 / (Range*Clock)
	pwmWrite(1, 0);
	// Create necessary directories for saving files
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video")
	fprintf(stdout, "Pi 1 is alive and running.\n");
	// TODO Implement Listening for communications from RXSM

	// Check for LO signal.

	while (digitalRead(LO))
		delay(10); // 10 ms delay to save CPU
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

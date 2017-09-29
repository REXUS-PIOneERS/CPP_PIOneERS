/*
 * Pi 1 is connected to the IMU and RTC via I2C, RXSM via UART and GPIO,
 * Camera via CSI, Motor via PWM and GPIO and Pi 2 via Ethernet.
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
UART RXSM = UART();
int IMU_data_stream;

// Ethernet communication setup and variables (Pi 2 acts as server)
std::string server_name = "PIOneERS2.local";
int port_no = 51717; // Random unused port for communication
int ethernet_streams[2];
Client ethernet_comms = Client(server_name, port_no);
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
	// Setup the IMU and start recording
	// TODO ensure IMU setup register values are as desired
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
		// TODO periodically send the count to ground
		piUnlock(1);
		delay(500);
	}
	piUnlock(1);
	pwmWrite(1, 0); // Stops sending pulses through the PWM pin.
	fprintf(stdout, "Info: Boom deployed!\n");
	fflush(stdout);

	// Wait for the next signal to continue the program
	while (digitalRead(SODS)) {
		// Read data from IMU_data_stream and send on to Ethernet
		int ch = 0;
		while ((ch = getc(ethernet_streams[0])) != 0 && n != 255)
			putc(ch, stdout); // TODO Need to echo characters to UART stream
		delay(50);
	}
	return SODS_SIGNAL();
}

int LO_SIGNAL() {
	/*
	 * When the 'Lift Off' signal is received from the REXUS Module the cameras
	 * are set to start recording video and we then wait to receive the 'Start
	 * of Experiment' signal (when the nose-cone is ejected)
	 */
	printf("Signal Received: LO\n");
	fflush(stdout);
	Cam.startVideo();
	fflush(stdout);
	// Poll the SOE pin until signal is received
	// TODO implement check to make sure no false signals!
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

	// Setup PWM
	pinMode(MOTOR_OUT, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(PWM_RANGE);
	pwmSetClock(PWM_CLOCK); //Freq = 19200000 / (Range*Clock)
	pwmWrite(1, 0);
	// Create necessary directories for saving files
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video");
	fprintf(stdout, "Pi 1 is alive and running.\n");
	// Wait for GPIO to go high signalling that Pi2 is ready to communicate
	while (!digitalRead(ALIVE))
		delay(10);

	ethernet_comms.run(ethernet_streams);
	// TODO handle error where we can't connect to he server

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

/*
 * Pi 2 is connected to the ImP/IMU via UART, Camera via CSI, Burn Wire Relay
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
#include "comms/pipes.h"
#include "comms/packet.h"

#include <wiringPi.h>

// Main inputs for experiment control
int LO = 29;
int SOE = 28;
int SODS = 27;

int LAUNCH_MODE_OUT = 10;
int LAUNCH_MODE_IN = 11;
bool flight_mode = false;

// Burn Wire Setup
int BURNWIRE = 4;

// Global variable for the Camera and IMU
PiCamera Cam = PiCamera();

// Setup for the UART communications
int baud = 230400;
UART ImP = UART();
comms::Pipe ImP_stream;

// Ethernet communication setup and variables (we are acting as client)
int port_no = 31415; // Random unused port for communication
comms::Pipe ethernet_stream;
Server ethernet_comms = Server(port_no);
int ALIVE = 3;

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

void signal_handler(int s) {
	std::cout << "Exiting program early..." << std::endl;
	Cam.stopVideo();
	if (&ethernet_stream != NULL) {
		delay(100);
		ethernet_stream.close_pipes();
	}
	if (&ImP_stream != NULL)
		ImP_stream.close_pipes();
	//system("sudo shutdown now");
	exit(1); // This was an unexpected end so we will exit with an error!
}

int SODS_SIGNAL() {
	/*
	 * When the 'Start of Data Storage' signal is received all data recording
	 * is stopped (IMU and Camera) and power to the camera is cut off to stop
	 * shorting due to melting on re-entry. All data is copied into a backup
	 * directory.
	 */
	std::cout << "SIGNAL: SODS" << std::endl;
	Cam.stopVideo();
	ethernet_stream.close_pipes();
	ImP_stream.close_pipes();
	std::cout << "Exiting Program" << std::endl;
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
	std::cout << "SIGNAL: SOE" << std::endl;
	// Setup the ImP and start requesting data
	ImP_stream = ImP.startDataCollection("Docs/Data/Pi2/test");
	comms::Packet p; // Buffer for reading data from the IMU stream
	// Trigger the burn wire!
	digitalWrite(BURNWIRE, 1);
	unsigned int start = millis();
	std::cout << "INFO: Burn wire triggered" << std::endl;
	while (1) {
		unsigned int time = millis() - start;
		if (time > 6000) break;
		// Get ImP data
		int n = ImP_stream.binread(&p, sizeof (p));
		if (n > 0) {
			std::cout << "DATA: " << p << std::endl;
			ethernet_stream.binwrite(&p, sizeof (p));
		}

		n = ethernet_stream.binread(&p, sizeof (p));
		if (n > 0)
			std::cout << "PI1: " << p << std::endl;
		delay(10);
	}
	digitalWrite(BURNWIRE, 0);
	std::cout << "INFO: Burn wire off" << std::endl << "Waiting for SIDS" <<
			std::endl;

	// Wait for the next signal to continue the program
	bool signal_received = false;
	while (!signal_received) {
		signal_received = poll_input(SODS);
		// Read data from IMU_data_stream and echo it to Ethernet
		int n = ImP_stream.binread(&p, sizeof (p));
		if (n > 0) {
			std::cout << "DATA: " << p << std::endl;
			ethernet_stream.binwrite(&p, sizeof (p));
		}

		n = ethernet_stream.binread(&p, sizeof (p));
		if (n > 0)
			std::cout << "PI1: " << p << std::endl;
		delay(10);
	}
	return SODS_SIGNAL();
}

int LO_SIGNAL() {
	/*
	 * When the 'Lift Off' signal is received from the REXUS Module the cameras
	 * are set to start recording video and we then wait to receive the 'Start
	 * of Experiment' signal (when the nose-cone is ejected)
	 */
	std::cout << "SIGNAL: LO" << std::endl;
	Cam.startVideo("Docs/Video/rexus_video");
	// Poll the SOE pin until signal is received
	std::cout << "Waiting for SOE" << std::endl;
	bool signal_received = false;
	while (!signal_received) {
		delay(10);
		signal_received = poll_input(SOE);
		// TODO Implement communications with RXSM
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
	// Create necessary directories for saving files
	std::cout << "Pi 2 is alive..." << std::endl;
	system("mkdir -p Docs/Data/Pi1 Docs/Data/Pi2 Docs/Data/test Docs/Video");
	wiringPiSetup();
	// Setup main signal pins
	pinMode(LO, INPUT);
	pullUpDnControl(LO, PUD_UP);
	pinMode(SOE, INPUT);
	pullUpDnControl(SOE, PUD_UP);
	pinMode(SODS, INPUT);
	pullUpDnControl(SODS, PUD_UP);
	pinMode(ALIVE, OUTPUT);

	// Setup pins and check whether we are in flight mode
	pinMode(LAUNCH_MODE_OUT, OUTPUT);
	pinMode(LAUNCH_MODE_IN, INPUT);
	pullUpDnControl(LAUNCH_MODE_IN, PUD_DOWN);
	digitalWrite(LAUNCH_MODE_OUT, 1);
	flight_mode = digitalRead(LAUNCH_MODE_IN);

	// Setup Burn Wire
	pinMode(BURNWIRE, OUTPUT);

	// Setup server and wait for client
	digitalWrite(ALIVE, 1);
	ethernet_stream = ethernet_comms.run("Docs/Data/Pi1/backup.txt");
	std::cout << "Connected to Pi 1" << std::endl << "Waiting for LO..." <<
			std::endl;

	// Check for LO signal.
	std::string msg;
	bool signal_received = false;
	comms::Packet p;
	comms::byte1_t id;
	comms::byte2_t index;
	comms::byte1_t data[16];
	while (!signal_received) {
		delay(10);
		signal_received = poll_input(LO);
		// TODO Implement communications with Pi 1
		int n = ethernet_stream.binread(&p, sizeof (p));
		if (n > 0) {
			comms::unpack(&p, &id, &index, data);
			if (id == comms::ID_MSG1) {
				std::string msg(data);
				std::cout << "MSG: " << msg << std::endl;
				if (msg.compare("RESTART") == 0)
					system("sudo reboot");
				else if (msg.compare("TEST"))
					std::cout << "INFO: Tests not yet implemented" << std::endl;
				else
					std::cout << "ERROR: Message not recognised" << std::endl;
			}
		}
	}
	LO_SIGNAL();
	//system("sudo reboot");
	return 0;
}

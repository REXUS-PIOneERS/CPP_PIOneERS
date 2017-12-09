/**
 * REXUS PIOneERS - Pi_1
 * tests.cpp
 * Purpose: Functions for testing the various modules of the REXUS PIOneERS
 * code. Each function returns 0 on success or an error code on failure.
 *
 * @author David Amison
 * @version 1.0 28/10/2017
 */

#include "tests.h"

#include "RPi_IMU/RPi_IMU.h"
#include <linux/i2c-dev.h>

#include "camera/camera.h"
#include "UART/UART.h"
#include "timing/timer.h"

#include "comms/pipes.h"
#include "comms/packet.h"
#include <iostream>

namespace tests {

	std::string all_tests() {
		std::string rtn = IMU_test();
		rtn += camera_test();
		return rtn;
	}

	std::string IMU_test() {
		std::string rtn = "\nTesting IMU...\n";
		RPi_IMU IMU;
		if (!IMU.setupAcc())
			rtn += "Failed to write to sensor.\n";
		else
			rtn += "Data written to sensor.\n";
		if (!IMU.readAccAxis(1))
			rtn += "Failed to read from sensor.\n";
		else
			rtn += "Value read from sensor.\n";
		// Test multiprocessing
		IMU.setupGyr();
		IMU.setupMag();
		comms::Pipe stream;
		stream = IMU.startDataCollection("imutest");
		Timer::sleep_ms(500);
		comms::Packet p;
		int n = 0;
		for (int i = 0; i < 5; i++) {
			if (stream.binread(&p, sizeof (p)) <= 0)
				rtn += "Stream not receiving data.\n";
			else
				rtn += "Stream receiving data.\n";
			Timer::sleep_ms(200);
		}
		stream.close_pipes();
		system("sudo rm -rf *.txt");
		return rtn;
		//std::fstream f("imutest0001.txt");
		//if (f.good()) {
		//	system("sudo rm -rf *.txt");
		//	return rtn + "Data saved to file.\n";
		//} else
		//	return rtn + "Data not saved to file.\n";
	}

	std::string camera_test() {
		std::string rtn = "\nTesting Camera...\n";
		PiCamera cam;
		cam.startVideo("camtest");
		Timer::sleep_ms(2500);
		// Check camera process is running
		if (!cam.is_running())
			return rtn + "Camera failed to start.\n";
		Timer::sleep_ms(2500);
		cam.stopVideo();
		system("sudo rm -rf *.h264");
		return rtn;
		// Check files were created
		//std::fstream f("camtest0001.h264");
		//if (f.good()) {
		//	system("sudo rm -rf *.h264");
		//	return rtn + "Camera tests passed.\n";
		//} else
		//	return rtn + "No files found.\n";
	}

	int ImP_test() {
		return 0;
	}

}
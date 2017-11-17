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

	int all_tests() {
		IMU_test();
		camera_test();
		return 0;
	}

	int IMU_test() {
		int rtn = 0;
		RPi_IMU IMU;
		if (!IMU.setupAcc())
			rtn |= 0b00000011;
		if (!IMU.readAccAxis(1))
			rtn |= 0b00000100;
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
				rtn |= 0b00001000;
			Timer::sleep_ms(200);
		}
		stream.close_pipes();
		std::fstream f("imutest0001.txt");
		if (f.good()) {
			system("sudo rm -rf *.txt");
			return rtn;
		} else
			return (rtn | 0b00001000);
	}

	int camera_test() {
		PiCamera cam;
		cam.startVideo("camtest");
		Timer::sleep_ms(2500);
		// Check camera process is running
		if (!cam.is_running())
			return 1;
		Timer::sleep_ms(2500);
		cam.stopVideo();
		// Check files were created
		std::fstream f("camtest0001.h264");
		if (f.good()) {
			system("sudo rm -rf *.h264");
			return 0;
		} else
			return 1;
	}

	int ImP_test() {
		return 0;
	}

}
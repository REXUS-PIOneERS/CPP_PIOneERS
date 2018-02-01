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
#include "pins1.h"
#include <iostream>
#include <wiringPi.h>

namespace tests {

	std::string pi1_tests() {
		std::string rtn = IMU_test();
		rtn += camera_test();
		return rtn;
	}

	std::string pi2_tests() {
		std::string rtn = ImP_test();
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
		return rtn + "Camera worked.\n";
		// Check files were created
		//std::fstream f("camtest0001.h264");
		//if (f.good()) {
		//	system("sudo rm -rf *.h264");
		//	return rtn + "Camera tests passed.\n";
		//} else
		//	return rtn + "No files found.\n";
	}

	std::string ImP_test() {
		std::string rtn = "\nTesting ImP...\n";
		comms::Packet p;
		comms::Pipe pipe;
		ImP imp(38400);
		pipe = imp.startDataCollection("imptest");
		Timer::sleep_ms(500);
		int n = pipe.binread(&p, sizeof(comms::Packet));
		if (n > 0)
			rtn += "ImP sending data.\n";
		else
			rtn += "ImP not sending data.\n";
		pipe.close_pipes();
		Timer::sleep_ms(500);
		system("sudo rm -rf *.txt");
		return rtn;
	}
  int motor_turn(int dir, int n, int *counter) {
    wiringPiSetup();
    pinMode(MOTOR_CW, OUTPUT);
    pinMode(MOTOR_ACW, OUTPUT);
    switch (dir) {
      case 0:
        digitalWrite(MOTOR_CW, 1);
      case 1:
        digitalWrite(MOTOR_ACW, 1);
    }
    int count;
    while (1) {
      piLock(1);
      count = *counter;
      piUnlock(1);
      if (count > n)
        break;
      Timer::sleep_ms(100);
    }
    digitalWrite(MOTOR_CW, 0);
    digitalWrite(MOTOR_ACW, 0);
    piLock(1);
    *counter = 0;
    piUnlock(1);
    return count;
  }

}

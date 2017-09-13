/*
 * File:   RPi_IMU.h
 * Author: David Amison
 *
 * Created on 29 March 2017, 14:11
 */

#ifndef BERRYIMU_H
#define BERRYIMU_H

#include <stdio.h>
#include <stdint.h>
#include "LSM9DS0.h"   //Stores addresses for the BerryIMU

#include <sys/types.h>

class RPi_IMU {
	char *filename = (char*) "/dev/i2c-1";
	int i2c_file = 0;
	pid_t pid; //Id of the background process

public:
	//Initialise the Class by opening the I2C bus
	RPi_IMU();
	//Functions for setting up various components
	bool setupAcc(int reg1_value = 0b01100111, int reg2_value = 0b00100000);
	//Default 100Hz, x, y and z active, 773 Hz	Anti-Alias, +/- 16g
	bool setupGyr(int reg1_value = 0b00001111, int reg2_value = 0b00110000);
	bool setupMag(int reg5_value = 0b11110000, int reg6_value = 0b11000000,
			int reg7_value = 0b00000000);

	bool writeReg(int addr, int reg, int value);

	//Functions to get results
	void readAcc(int16_t *data);
	void readGyr(int16_t *data);
	void readMag(int16_t *data);
	int16_t readAccAxis(int axis);
	int16_t readGyrAxis(int axis);
	int16_t readMagAxis(int axis);

	int startDataCollection(char* filename);
	int stopDataCollection();

	~RPi_IMU() {
		resetRegisters();
		// close(i2c_file);
	}

private:
	bool activateSensor(int addr);
	//Functions to activate various sensors

	bool activateAcc() {
		return activateSensor(ACC_ADDRESS);
	}

	bool activateGyr() {
		return activateSensor(GYR_ADDRESS);
	}

	bool activateMag() {
		return activateSensor(MAG_ADDRESS);
	}
	void resetRegisters();
};

#endif /* BERRYIMU_H */


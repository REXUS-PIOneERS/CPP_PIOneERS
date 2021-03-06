/**
 * REXUS PIOneERS - Pi_1
 * RPi_IMU.h
 * Purpose: Class definition for controlling the BerryIMU
 *
 * @author David Amison
 * @version 2.5 10/10/2017
 */

#ifndef BERRYIMU_H
#define BERRYIMU_H

#include <stdio.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "LSM9DS1.h"   //Stores addresses for the BerryIMU
#include "comms/pipes.h"
#include "comms/packet.h"

#include "logger/logger.h"

#include <sys/types.h>

class RPi_IMU {
	char *filename = (char*) "/dev/i2c-1";
	int i2c_file = 0;
	int _pid; //Id of the background process
	bool _bus_active = false;
	comms::Pipe _pipes;
	Logger Log;

public:

	/**
	 * Default constructor opens the i2c file ready for communication.
	 */
	RPi_IMU() : Log("/Docs/Logs/imu") {
		Log.start_log();
		//Open the I2C bus
		Log("INFO") << "Attempting to open i2c bus";
		if ((i2c_file = open(filename, O_RDWR)) < 0) {
			Log("ERROR") << "Failed to open i2c bus";
			_bus_active = false;
		} else {
			_bus_active = true;
		}
	}

	/**
	 * Sets up the accelerometer registers. See BerryIMU documentation for
	 * details on the effect of different values.
	 * 
	 * Defaults: x y and z enabled, Data Rate 50 Hz, BW 50 Hz, +- 2g
	 *
	 * @param reg5_value: Value written to CTRL_REG5_XL
	 * @param reg6_value: Value written to CTRL_REG6_XL
	 * @return true: write successful, false: write failed
	 */
	bool setupAcc(int reg5_value = 0b00111000, int reg6_value = 0b01000000);

	/**
	 * Sets up the gyroscope registers. See BerryIMU documentation for details
	 * on the effect of different values.
	 * 
	 * Defaults: 238 Hz, 500 dps, 14 Hz cutoff, x y and z enabled, no interrupts
	 *
	 * @param reg1_value: Value written to CTRL_REG1_G
	 * @param reg4_value: Value written to CTRL_REG4
	 * @param reg_orient_value: Value written to ORIENT_CFG_G
	 * @return true: write successful, false: write failed
	 */
	bool setupGyr(int reg1_value = 0b01011000, int reg4_value = 0b00111000,
	int reg_orient_value = 0b00000000);

	/**
	 * Sets up the magnetometer registers. See BerryIMU documentation for
	 * details on the effect of different values.
	 * 
	 * Defaults: High-performance, 59.5 Hz ODR, +- 4 gauss, continuous conversian
	 *
	 * @param reg1_value: Value written to CTRL_REG1_M
	 * @param reg2_value: Value written to CTRL_REG2_M
	 * @param reg3_value: Value written to CTRL_REG3_M
	 * @param reg4_value: Value written to CTRL_REG4_M
	 * @return true: write successful, false: write failed
	 */
	bool setupMag(int reg1_value = 0b11110100, int reg2_value = 0b00000000,
			int reg3_value = 0b00000000, int reg4_value = 0b00001100);

	/**
	 * Write a value to a register.
	 *
	 * @param addr: Address of the device
	 * @param reg: Register to write to
	 * @param value: Value to write to register
	 * @return true: write successful, false: write failed
	 */
	bool writeReg(int addr, int reg, int value);

	/**
	 * Read the x, y and z valued of the accelerometer
	 * @param data: Array of size 3 to store return values
	 */
	void readAcc(uint16_t *data);

	/**
	 * Read the x, y and z valued of the gyro
	 * @param data: Array of size 3 to store return values
	 */
	void readGyr(uint16_t *data);

	/**
	 * Read the x, y and z valued of the magnetometer
	 * @param data: Array of size 3 to store return values
	 */
	void readMag(uint16_t *data);

	/**
	 * Read accelerometer data from one axis
	 * @param axis: 1=x, 2=y, 3=z
	 * @return Value read from accelerometer
	 */
	uint16_t readAccAxis(int axis);

	/**
	 * Read gyro data from one axis
	 * @param axis: 1=x, 2=y, 3=z
	 * @return Value read from gyro
	 */
	uint16_t readGyrAxis(int axis);

	/**
	 * Read magnetometer data from one axis
	 * @param axis: 1=x, 2=y, 3=z
	 * @return Value read from magnetometer
	 */
	uint16_t readMagAxis(int axis);

	/**
	 * Read data from all axes of acc, gyro and mag.
	 * @param data: Array of size 9 to store data
	 */
	void readRegisters(comms::byte1_t *data);

	comms::Pipe startDataCollection(char* filename);

	bool status();

	int stopDataCollection();
	
	void resetRegisters();

	~RPi_IMU() {
		Log("INFO") << "Destroying IMU object";
		Log.stop_log();
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
};

#endif /* BERRYIMU_H */


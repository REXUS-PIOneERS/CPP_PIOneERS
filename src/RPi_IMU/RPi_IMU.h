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

#include "LSM9DS0.h"   //Stores addresses for the BerryIMU
#include "comms/pipes.h"
#include "comms/packet.h"

#include "logger/logger.h"

#include <sys/types.h>

class RPi_IMU {
	char *filename = (char*) "/dev/i2c-1";
	int i2c_file = 0;
	int pid; //Id of the background process
	comms::Pipe m_pipes;
	Logger log;

public:

	/**
	 * Default constructor opens the i2c file ready for communication.
	 */
	RPi_IMU() : log("/Docs/Logs/imu") {
		log.start_log();
		//Open the I2C bus
		log("INFO") << "Attempting to open i2c bus";
		if ((i2c_file = open(filename, O_RDWR)) < 0) {
			log("ERROR") << "Failed to open i2c bus";
		}
	}

	RPi_IMU& operator=(RPi_IMU &in) {
		log = in.log;
		filename = in.filename;
		i2c_file = in.i2c_file;
		pid = in.pid;
		m_pipes = in.m_pipes;
	}
	/**
	 * Sets up the accelerometer registers. See BerryIMU documentation for
	 * details on the effect of different values.
	 *
	 * @param reg1_value: Value written to CTRL_REG1_XM
	 * @param reg2_value: Value written to CTRL_REG2_XM
	 * @return true: write successful, false: write failed
	 */
	bool setupAcc(int reg1_value = 0b01100111, int reg2_value = 0b00100000);

	/**
	 * Sets up the gyroscope registers. See BerryIMU documentation for details
	 * on the effect of different values.
	 *
	 * @param reg1_value: Value written to CTRL_REG1_G
	 * @param reg2_value: Value written to CTRL_REG2_G
	 * @return true: write successful, false: write failed
	 */
	bool setupGyr(int reg1_value = 0b00001111, int reg2_value = 0b00110000);

	/**
	 * Sets up the magnetometer registers. See BerryIMU documentation for
	 * details on the effect of different values.
	 *
	 * @param reg5_value: Value written to CTRL_REG5_XM
	 * @param reg6_value: Value written to CTRL_REG6_XM
	 * @param reg7_value: Value written to CTRL_REG7_XM
	 * @return true: write successful, false: write failed
	 */
	bool setupMag(int reg5_value = 0b11110000, int reg6_value = 0b11000000,
			int reg7_value = 0b00000000);

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
	int stopDataCollection();

	~RPi_IMU() {
		log("INFO") << "Destroying IMU object";
		log.stop_log();
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


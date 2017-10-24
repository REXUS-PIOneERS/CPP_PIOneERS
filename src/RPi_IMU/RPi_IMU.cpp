/**
 * REXUS PIOneERS - Pi_1
 * RPi_IMU.cpp
 * Purpose: Implementation of functions for controlling the BerryIMU as defined
 *		in RPi_IMU class
 *
 * @author David Amison
 * @version 2.5 10/10/2017
 */

#include "RPi_IMU.h"

#include <stdio.h>
#include <stdint.h>
#include <iostream>
//Includes for multiprocessing
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>  // For waitpid

// For communication and data packing
#include "comms/pipes.h"
#include "comms/transceiver.h"
#include "comms/packet.h"

#include <string>
#include "timing/timer.h"
#include <fstream>  //For writing to files

// Includes for I2c
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "LSM9DS0.h"

RPi_IMU::RPi_IMU() {
	//Open the I2C bus
	if ((i2c_file = open(filename, O_RDWR)) < 0) {
		printf("Failed to open I2C bus");
	}
}

bool RPi_IMU::activateSensor(int addr) {
	if (ioctl(i2c_file, I2C_SLAVE, addr) < 0) {
		printf("Failed to aquire bus access and/or talk to slave");
		return false;
	}
	return true;
}

//Functions for setting up the various sensors

bool RPi_IMU::setupAcc(int reg1_value, int reg2_value) {
	//Write the values to the control registers
	bool a = writeReg(ACC_ADDRESS, CTRL_REG1_XM, reg1_value);
	bool b = writeReg(ACC_ADDRESS, CTRL_REG2_XM, reg2_value);
	return (a && b);
}

bool RPi_IMU::setupGyr(int reg1_value, int reg2_value) {
	bool a = writeReg(GYR_ADDRESS, CTRL_REG1_G, reg1_value);
	bool b = writeReg(GYR_ADDRESS, CTRL_REG2_G, reg2_value);
	return (a && b);
}

bool RPi_IMU::setupMag(int reg5_value, int reg6_value, int reg7_value) {
	bool a = writeReg(MAG_ADDRESS, CTRL_REG5_XM, reg5_value);
	bool b = writeReg(MAG_ADDRESS, CTRL_REG6_XM, reg6_value);
	bool c = writeReg(MAG_ADDRESS, CTRL_REG7_XM, reg7_value);
	return (a && b && c);
}

bool RPi_IMU::writeReg(int addr, int reg, int value) {
	try {
		activateSensor(addr);

		int result = i2c_smbus_write_byte_data(i2c_file, reg, value);
		if (result == -1) {
			printf("Failed to write byte to I2C register");
			return false;
		}
		return true;

	} catch (int) {
		printf("Failed to activate sensor");
		return false;
	}
}

uint16_t RPi_IMU::readAccAxis(int axis) {
	//Select the device
	activateSensor(ACC_ADDRESS);
	//axis is either 1:x, 2:y, 3:z
	int reg1, reg2;
	switch (axis) {
		case 1:
			reg1 = OUT_X_L_A;
			reg2 = OUT_X_H_A;
			break;
		case 2:
			reg1 = OUT_Y_L_A;
			reg2 = OUT_Y_H_A;
			break;
		case 3:
			reg1 = OUT_Z_L_A;
			reg2 = OUT_Z_H_A;
			break;
		default:
			return 0;
	}
	//Read from the registers
	uint8_t Acc_L = i2c_smbus_read_word_data(i2c_file, reg1);
	uint8_t Acc_H = i2c_smbus_read_word_data(i2c_file, reg2);
	uint16_t data = (uint16_t) (Acc_L | Acc_H << 8);

	return data;
}

uint16_t RPi_IMU::readGyrAxis(int axis) {
	//Select the device
	activateSensor(GYR_ADDRESS);
	//axis is either 1:x, 2:y, 3:z
	int reg1, reg2;
	switch (axis) {
		case 1:
			reg1 = OUT_X_L_G;
			reg2 = OUT_X_H_G;
			break;
		case 2:
			reg1 = OUT_Y_L_G;
			reg2 = OUT_Y_H_G;
			break;
		case 3:
			reg1 = OUT_Z_L_G;
			reg2 = OUT_Z_H_G;
			break;
		default:
			return 0;
	}
	//Read from the registers
	uint8_t Gyr_L = i2c_smbus_read_word_data(i2c_file, reg1);
	uint8_t Gyr_H = i2c_smbus_read_word_data(i2c_file, reg2);
	uint16_t data = (uint16_t) (Gyr_L | Gyr_H << 8);

	return data;
}

uint16_t RPi_IMU::readMagAxis(int axis) {
	//Select the device
	activateSensor(MAG_ADDRESS);
	//axis is either 1:x, 2:y, 3:z
	int reg1, reg2;
	switch (axis) {
		case 1:
			reg1 = OUT_X_L_M;
			reg2 = OUT_X_H_M;
			break;
		case 2:
			reg1 = OUT_Y_L_M;
			reg2 = OUT_Y_H_M;
			break;
		case 3:
			reg1 = OUT_Z_L_M;
			reg2 = OUT_Z_H_M;
			break;
		default:
			return 0;
	}
	//Read from the registers
	uint8_t Gyr_L = i2c_smbus_read_word_data(i2c_file, reg1);
	uint8_t Gyr_H = i2c_smbus_read_word_data(i2c_file, reg2);
	uint16_t data = (uint16_t) (Gyr_L | Gyr_H << 8);

	return data;
}



//Functions for reading data

void RPi_IMU::readAcc(uint16_t *data) {
	//Select the accelerometer
	activateSensor(ACC_ADDRESS);
	//Request reading from the accelerometer
	uint8_t block[6];
	uint8_t *block_addr = block;
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_A, sizeof (block), block_addr);
	//Calculate x, y and z values
	data[0] = (uint16_t) (block[0] | block[1] << 8);
	data[1] = (uint16_t) (block[2] | block[3] << 8);
	data[2] = (uint16_t) (block[4] | block[5] << 8);

	return;
}

void RPi_IMU::readGyr(uint16_t *data) {
	//Select the gyro sensor
	activateSensor(GYR_ADDRESS);
	//Request reading from the gyro
	uint8_t block[6];
	uint8_t *block_addr = block;
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_G, sizeof (block), block_addr);
	//Calculate x, y and z values
	data[0] = (uint16_t) (block[0] | block[1] << 8);
	data[1] = (uint16_t) (block[2] | block[3] << 8);
	data[2] = (uint16_t) (block[4] | block[5] << 8);

	return;
}

void RPi_IMU::readMag(uint16_t *data) {
	//Select the magnetometer
	activateSensor(MAG_ADDRESS);
	//Request data
	uint8_t block[6];
	uint8_t *block_addr = block;
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_M, sizeof (block), block_addr);
	data[0] = (uint16_t) (block[0] | block[1] << 8);
	data[1] = (uint16_t) (block[2] | block[3] << 8);
	data[2] = (uint16_t) (block[4] | block[5] << 8);

	return;
}

void RPi_IMU::readRegisters(comms::byte1_t *data) {
	// Read all registers for accelerometer, magnetometer and gyroscope
	int n = 0;
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_A, 6, data);
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_G, 6, (data + 6));
	i2c_smbus_read_i2c_block_data(i2c_file, 0x80 | OUT_X_L_M, 6, (data + 12));
}

void RPi_IMU::resetRegisters() {
	//Set all registers in the IMU to 0
	writeReg(ACC_ADDRESS, CTRL_REG1_XM, 0);
	writeReg(ACC_ADDRESS, CTRL_REG2_XM, 0);
	writeReg(GYR_ADDRESS, CTRL_REG1_G, 0);
	writeReg(GYR_ADDRESS, CTRL_REG2_G, 0);
	writeReg(MAG_ADDRESS, CTRL_REG5_XM, 0);
	writeReg(MAG_ADDRESS, CTRL_REG6_XM, 0);
	writeReg(MAG_ADDRESS, CTRL_REG7_XM, 0);
}

comms::Pipe RPi_IMU::startDataCollection(char* filename) {
	try {
		m_pipes = comms::Pipe();
		if ((pid = m_pipes.Fork()) == 0) {
			// This is the child process and controls data collection
			comms::Packet p1;
			comms::Packet p2;
			comms::byte1_t data[22];
			int intv = 200;
			Timer measurement_time;
			// Infinite loop for taking measurements
			for (int j = 0;; j++) {
				// Open the file for saving data
				std::ofstream outf;
				char unique_file[50];
				sprintf(unique_file, "%s%04d.txt", filename, j);
				outf.open(unique_file);
				// Take 5 measurements i.e. 1 seconds worth of data
				for (int i = 0; i < 5; i++) {
					Timer tmr;
					readRegisters(data);
					int64_t time = measurement_time.elapsed();
					data[18] = (time & 0x00000011);
					data[19] = (time & 0x00001100) >> 8;
					data[20] = (time & 0x00110000) >> 16;
					data[21] = (time & 0x11000000) >> 24;
					// Output data to the file (all one line)
					outf << time << "," << "Acc," << data[0] << "," <<
							data[1] << "," << data[2] << "Gyr," <<
							data[0] << "," << data[1] << "," <<
							data[2] << "Mag," << data[0] << "," <<
							data[1] << "," << data[2] << std::endl;
					//write(dataPipe[1], 0x00, 1);
					comms::Protocol::pack(p1, 0b00010000, (5 * j) + 1, data);
					comms::Protocol::pack(p2, 0b00010001, (5 * j) + i, data + 12);
					m_pipes.binwrite(&p1, sizeof (p1));
					m_pipes.binwrite(&p2, sizeof (p2));
					while (tmr.elapsed() < intv)
						tmr.sleep_ms(10);
				}
				// Close the current file, ready to start a new one
				outf.close();
			}
		} else {
			// This is the parent process
			return m_pipes; // Return the read portion of the pipe
		}
	} catch (comms::PipeException e) {
		// Ignore a broken pipe and exit silently
		resetRegisters();
		m_pipes.close_pipes();
		fprintf(stdout, "IMU %s\n", e.what());
		exit(0); // Happily end the process
		// TODO handle different types of exception!
	} catch (...) {
		resetRegisters();
		m_pipes.close_pipes();
		perror("ERROR with IMU");
		exit(1);
	}
}

int RPi_IMU::stopDataCollection() {
	if (pid)
		m_pipes.close_pipes();
	return 0;
}


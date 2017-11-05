/**
 * REXUS PIOneERS - Pi_1
 * UART.cpp
 * Purpose: Function implementations for the UART class
 *
 * @author David Amison
 * @version 3.2 12/10/2017
 */
#include "UART.h"

#include <stdio.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>  //Used for UART
#include <termios.h> //Used for UART
// For multiprocessing
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <fstream>
#include <string>
#include "UART.h"
#include "comms/packet.h"
#include "comms/pipes.h"
#include "comms/transceiver.h"
#include "comms/protocol.h"

#include "timing/timer.h"

#include "logger/logger.h"
#include <error.h>

void UART::setupUART() {
	//Open the UART in non-blocking read/write mode
	uart_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY);
	if (uart_filestream == -1) {
		//throw UARTException("ERROR opening serial port");
		return;
	}
	speed_t f_baud;
	switch (_baudrate) {
		case 9600:
			f_baud = B9600;
			break;
		case 19200:
			f_baud = B19200;
			break;
		case 38400:
			f_baud = 38400;
			break;
		case 57600:
			f_baud = 57600;
			break;
		case 115200:
			f_baud = 115200;
			break;
		case 230400:
			f_baud = 230400;
			break;
		default:
			throw UARTException("ERROR baud rate not defined");
	}
	//Configure the UART
	struct termios options;
	tcgetattr(uart_filestream, &options);
	options.c_cflag = f_baud | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart_filestream, TCIFLUSH);
	tcsetattr(uart_filestream, TCSANOW, &options);
}

UART::~UART() {
	close(uart_filestream);
}

void RXSM::buffer() {
	try {
		_pipes = comms::Pipe();
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process
			Log.child_log();
			comms::Packet p;
			int n;
			while (1) {
				n = _pipes.binread(&p, sizeof (p));
				if (n > 0)
					sendPacket(p);
				Timer::sleep_ms(10);

				n = recvPacket(p);
				if (n > 0)
					_pipes.binwrite(&p, sizeof (p));
				Timer::sleep_ms(10);
			}
		} else {
			// This is the parent process
			return;
		}
	} catch (comms::PipeException e) {
		Log("FATAL") << "Failed to read or write to pipe\n\t" << e.what();
		Log("INFO") << "Shutting down communication with RXSM";
		_pipes.close_pipes();
		exit(-1);
	} catch (...) {
		Log("FATAL") << "Unknown exception with RXSM\n\t" << std::strerror(errno);
		Log("INFO") << "Shutting down communication with RXSM";
		_pipes.close_pipes();
		exit(-2);
	}
}

void RXSM::end_buffer() {
	if (_pid)
		_pipes.close_pipes();
}

int RXSM::sendPacket(comms::Packet &p) {
	Log("SENT") << p;
	if (_pid)
		return _pipes.binwrite(&p, sizeof (p));
	else
		return comms::Transceiver::sendPacket(&p);
}

int RXSM::recvPacket(comms::Packet &p) {
	int n;
	if (_pid)
		n = _pipes.binread(&p, sizeof (p));
	else
		n = comms::Transceiver::recvPacket(&p);
	if (n > 0)
		Log("RECEIVED") << p;
	return n;
}

int RXSM::sendMsg(std::string msg) {
	int n = msg.length();
	int sent = 0;
	comms::Packet p;
	for (int i = 0; i < n; i += 15) {
		std::string data = msg.substr(i, 15);
		comms::Protocol::pack(p, ID_MSG1, _index++, &data);
		sent += sendPacket(p);
	}
	return sent;
}

comms::Pipe ImP::startDataCollection(const std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	Log("INFO") << "Starting ImP and IMU data collection";
	try {
		_pipes = comms::Pipe();
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process
			Log.child_log();
			// Infinite loop for data collection
			comms::Transceiver ImP_comms(uart_filestream);
			// Send initial start command
			ImP_comms.sendBytes("C", 1);
			Log("DATA (SENT") << "C";
			for (int j = 0;; j++) {
				std::ofstream outf;
				char unique_file[50];
				sprintf(unique_file, "%s%04d.txt", filename.c_str(), j);
				Log("INFO") << "Starting new data file \"" << unique_file << "\"";
				outf.open(unique_file);
				// Take five measurements then change the file
				int intv = 200;
				for (int i = 0; i < 5; i++) {
					Timer tmr;
					char buf[256];
					// Wait for data to come through
					while (1) {
						int n = ImP_comms.recvBytes(buf, 255);
						/*
						 * It is assumed that the data is received in the following format
						 * Byte 1-6 Accelerometer (x, y, z)
						 * Byte 7-12 Gyroscope (x, y, z)
						 * Byte 13-18 Magnetometer (x, y, z)
						 * Byte 19-22 time
						 * Byte 23-24 ImP Measurement
						 */
						if (n > 0) {
							comms::Packet p1;
							comms::Packet p2;
							comms::byte1_t id1 = 0b00100000;
							comms::byte1_t id2 = 0b00100010;
							comms::byte2_t index = (5 * j) + i;
							comms::Protocol::pack(p1, id1, index, buf);
							comms::Protocol::pack(p2, id2, index, buf + 12);
							_pipes.binwrite(&p1, sizeof (p1));
							_pipes.binwrite(&p2, sizeof (p2));
							buf[n] = '\0';
							for (int i = 0; i < n; i++)
								outf << (int) buf[n] << ",";
							outf << std::endl;
							Log("DATA (IMP)") << p1;
							Log("DATA (ImP)") << p2;
							ImP_comms.sendBytes("N", 1);
							Log("DATA (SENT)") << "N";
							break;
						}
					}
					while (tmr.elapsed() < intv)
						tmr.sleep_ms(10);
				}
			}
		} else {
			return _pipes;
		}
	} catch (comms::PipeException e) {
		Log("FATAL") << "Unable to read/write to pipes\n\t\"" << e.what() << "\"";
		_pipes.close_pipes();
		close(uart_filestream);
		exit(-1);
	} catch (...) {
		Log("FATAL") << "Unexpected error with ImP\n\t\"" << std::strerror(errno);
		_pipes.close_pipes();
		close(uart_filestream);
		exit(-2);
	}
}

int ImP::stopDataCollection() {
	Log("INFO") << "Ending data collection by closing pipes";
	if (_pid)
		_pipes.close_pipes();
	return 0;
}

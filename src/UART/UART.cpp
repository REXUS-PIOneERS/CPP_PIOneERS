/**
 * REXUS PIOneERS - Pi_1
 * UART.cpp
 * Purpose: Function implementations for the UART class
 *
 * @author David Amison
 * @version 2.2 12/10/2017
 */

#include <stdio.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>  //Used for UART
#include <termios.h> //Used for UART
// For multiprocessing
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "UART.h"
#include <fstream>
#include <string>
#include "comms/packet.h"
#include "comms/pipes.h"
#include "comms/transceiver.h"
#include "comms/protocol.h"
#include "timing/timer.h"
#include <wiringPi.h>

void UART::setupUART() {
	//Open the UART in non-blocking read/write mode
	uart_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY);
	if (uart_filestream == -1)
		throw UARTException("ERROR opening serial port");
	//Configure the UART
	struct termios options;
	tcgetattr(uart_filestream, &options);
	options.c_cflag = B230400 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart_filestream, TCIFLUSH);
	tcsetattr(uart_filestream, TCSANOW, &options);
}

comms::Pipe UART::startDataCollection(const std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	try {
		m_pipes = comms::Pipe();
		if ((m_pid = m_pipes.Fork()) == 0) {
			// This is the child process
			// Infinite loop for data collection
			comms::Transceiver ImP_comms(uart_filestream);
			// Send initial start command
			ImP_comms.sendBytes("C", 1);
			for (int j = 0;; j++) {
				std::ofstream outf;
				char unique_file[50];
				sprintf(unique_file, "%s%04d.txt", filename.c_str(), j);
				outf.open(unique_file);
				// Take five measurements then change the file
				for (int i = 0; i < 5; i++) {
					uint64_t start = millis();
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
							m_pipes.binwrite(&p1, sizeof (p1));
							m_pipes.binwrite(&p2, sizeof (p2));
							buf[n] = '\0';
							outf << std::string(buf) << std::endl;
							ImP_comms.sendBytes("N", 1);
							break;
						}
					}
					while ((millis() - start) < 200)
						delay(10);
				}
			}
		} else {
			return m_pipes;
		}
	} catch (comms::PipeException e) {
		fprintf(stdout, "UART %s\n", e.what());
		m_pipes.close_pipes();
		close(uart_filestream);
		exit(0);
	} catch (...) {
		perror("ERROR with ImP");
		m_pipes.close_pipes();
		close(uart_filestream);
		exit(1);
	}
}

int UART::stopDataCollection() {
	if (m_pid)
		m_pipes.close_pipes();
	return 0;
}

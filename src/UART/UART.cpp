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
#include "pipes/pipes.h"
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

int UART::sendBytes(const void *buf, int n) {
	int sent = write(uart_filestream, (void*) buf, n);
	if (sent < 0) {
		throw UARTException("ERROR sending bytes");
		return -1;
	}
	return 0;
}

int UART::getBytes(void* buf, int n) {
	/*
	 * Gets bytes that are waiting in the UART stream (max 255 bytes)
	 */
	int buf_length = read(uart_filestream, buf, n);
	if (buf_length < 0) {
		throw UARTException("ERROR reading bytes");
		return -1;
	} else if (buf_length == 0) {
		//There was no data to read
		return 0;
	} else
		return buf_length;
}

Pipe UART::startDataCollection(const std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	try {
		m_pipes = Pipe();
		if ((m_pid = m_pipes.Fork()) == 0) {
			// This is the child process
			// Infinite loop for data collection
			sendBytes("C", 1);
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
						int n = getBytes(buf, 255);
						if (n > 0) {
							buf[n] = '\0';
							m_pipes.binwrite(buf, n);
							outf << std::string(buf) << std::endl;
							sendBytes("N", 1);
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
	} catch (PipeException e) {
		fprintf(stdout, "UART %s\n", e.what());
		sendBytes("S", 1);
		m_pipes.close_pipes();
		close(uart_filestream);
		exit(0);
	} catch (UARTException e) {
		fprintf(stdout, "%s\n", e.what());
		m_pipes.close_pipes();
		close(uart_filestream);
		exit(1);
	} catch (...) {
		perror("ERROR with ImP");
		sendBytes("S", 1);
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

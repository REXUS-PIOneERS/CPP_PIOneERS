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

void UART::setupUART() {
	//Open the UART in non-blocking read/write mode
	uart_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);
	if (uart_filestream == -1) {
		//ERROR: Failed to open serial port!
		fprintf(stderr, "Error: unable to open serial port. Ensure it is correctly set up\n");
	}
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

int UART::sendBytes(char *buf, int n) {
	int sent = write(uart_filestream, (void*) buf, n);
	if (sent < 0) {
		fprintf(stderr, "Failed to send bytes");
		return -1;
	}
	return 0;
}

int UART::getBytes(char* buf, int n) {
	/*
	 * Gets bytes that are waiting in the UART stream (max 255 bytes)
	 */
	int buf_length = read(uart_filestream, buf, n);
	if (buf_length < 0) {
		fprintf(stderr, "Error: Unable to read bytes");
		return -1;
	} else if (buf_length == 0) {
		//There was no data to read
		return 0;
	} else
		return buf_length;
}

Pipe UART::startDataCollection(std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	try {
		Pipe pipes = Pipe();
		if ((m_pid = pipes.Fork()) == 0) {
			// This is the child process
			// Infinite loop for data collection
			for (int j = 0;; j++) {
				std::ofstream outf;
				char unique_file[50];
				sprintf(unique_file, "%s%04d.txt", filename.c_str(), j);
				outf.open(unique_file);
				sendBytes("C", 1);
				// Take five measurements then change the file
				for (int i = 0; i < 5; i++) {
					char buf[256];
					// Wait for data to come through
					while (1) {
						int n = getBytes(buf, 255);
						if (n > 0) {
							buf[n] = '\0';
							pipes.binwrite(buf, n);
							outf << buf << std::endl;
							sendBytes("N", 1);
						}
					}
				}
			}
		} else {
			return pipes;
		}
	} catch (PipeException e) {
		sendBytes("S", 1);
		fprintf(stdout, "%s\n", e.what());
		close(uart_filestream);
		exit(0);
	} catch (...) {
		perror("ERROR with ImP");
		close(uart_filestream);
		exit(1);
	}
}

int UART::stopDataCollection() {
	if (m_pid) {
		bool died = false;
		fprintf(stdout, "Stopping IMU and ImP... ID:%d\n", m_pid);
		for (int i = 0; !died && i < 5; i++) {
			int status;
			kill(m_pid, SIGTERM);
			sleep(1);
			if (waitpid(m_pid, &status, WNOHANG) == m_pid) died = true;
		}
		if (died) {
			fprintf(stdout, "IMU and ImP Terminated\n");
		} else {
			fprintf(stdout, "IMU and ImP Killed\n");
			kill(m_pid, SIGKILL);
		}
		sendBytes("S", 1);
		close(uart_filestream);
	}
	return 0;
}

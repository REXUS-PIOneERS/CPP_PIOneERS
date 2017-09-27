#include <stdio.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>  //Used for UART
#include <termios.h> //Used for UART
// For multiprocessing
#include <signal>
#include <sys/wait.h>
#include <stdlib.h>
#include "UART.h"

void UART::setupUART() {
	//Open the UART in non-blocking read/write mode
	uart_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY);
	if (uart_filestream == -1) {
		//ERROR: Failed to open serial port!
		fprintf(stderr, "Error: unable to open serial port. Ensure it is correctly set up\n");
	}
	//Configure the UART
	struct termios options;
	tcgetattr(uart_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart_filestream, TCIFLUSH);
	tcsetattr(uart_filestream, TCSANOW, &options);
}

int UART::sendBytes(unsigned char *buf, size_t count) {
	int sent = write(uart_filestream, (void*) buf, count);
	if (sent < 0) {
		fprintf(stderr, "Failed to send bytes");
		return -1;
	}
	return 0;
}

int UART::getBytes(unsigned char buf[256]) {
	/*
	 * Gets bytes that are waiting in the UART stream (max 255 bytes)
	 */
	int buf_length = read(uart_filestream, (void*) buf, 255);
	if (buf_length < 0) {
		fprintf(stderr, "Error: Unable to read bytes");
		return -1;
	} else if (buf_length == 0) {
		//There was no data to read
		return 0;
	} else
		return buf_length;
}

int UART::startDataCollection(std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	int dataPipe[2];
	pipe(dataPipe);

	if ((m_pid = fork()) == 0) {
		// This is the child process
		close(dataPipe[0]); // Not needed
		// Infinite loop for data collection
		sendBytes('\0', 1);
		for (int j = 0;; j++) {
			std::ofstream outf;
			char unique_file[50];
			sprintf(unique_file, "%s%04d.txt", filename, j);
			outf.open(unique_file);
			sendBytes('N', 1);
			// Take five measurements then change the file
			for (int i = 0; i < 5; i++) {
				char buf[256];
				// Wait for data to come through
				while (1) {
					int i = 0;
					buf[i] = getc(uart_filestream);
					// Check if we actually have some data or got nothing
					if (feof(uart_filestream)) continue;
					// Get the data till the next break
					while ((buf[++i] = getc(uart_filestream)) != '\0') continue;
					break;
				}
				// Send the data back to the main program and write to file
				write(dataPipe[1], buf, strlen(buf));
				fprintf(unique_file, "%s%s", buf, "\n");
			}
		}
	} else {
		close(dataPipe[1]);
		return dataPipe[0];
	}
	return -1; // This should never happen.
}

int UART::stopDataCollection() {
	if (m_pid) {
		bool died = false;
		fprintf(stdout, "Stopping IMU and ImP... ID:%d\n", m_pid);
		for (int i = 0; !died && i < 5; i++) {
			int status;
			kill(m_pid, SIGTERM);
			sleep(1);
			if (waitpid(m_pid, &status, WHOHANG) == m_pid) died = true;
		}
		if (died) {
			fprintf(stdout, "IMU and ImP Terminated\n");
		} else {
			fprintf(stdout, "IMU and ImP Killed\n");
			kill(m_pid, SIGKILL);
		}
		UART::sendBytes('S', 1);
		close(uart_filestream);
	}
	return 0;
}

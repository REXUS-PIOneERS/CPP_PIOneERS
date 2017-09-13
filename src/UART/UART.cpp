#include <stdio.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>  //Used for UART
#include <termios.h> //Used for UART
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

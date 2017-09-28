/*
 * Simple file to test functionality of the UART to get data from the ImP
 */
#include <Uart/UART.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include <wiringPi.h>

// Pins for starting and stopping data collection
int START = 3;
int STOP = 4;
UART ImP = UART();

int main() {
	wiringPiSetup();
	pinMode(3, INPUT);
	pinMode(4, INPUT);
	// Wait for low signal to request ImP measurements
	while (digitalRead(3)) delay(10);

	int data_stream = ImP.startDataCollection("test");
	char buf[256];
	delay(500);
	FILE* fd = fdopen(data_stream, 'r');
	for (i = 0; i < 50; i++) {
		int n;
		while (1) {
			char c = getc(fd);
			if (c != EOF) putc(c, stdout);
		}
		fprintf(cout, "%s%s", buf, "\n");
	}

	return 0;
}
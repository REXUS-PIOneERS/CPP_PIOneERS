/*
 * Simple file to test functionality of the UART to get data from the ImP
 */
#include <UART/UART.h>
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
	pinMode(START, INPUT);
	pinMode(STOP, INPUT);
	pullUpDnControl(START, PUD_UP);
	pullUpDnControl(STOP, PUD_UP);
	fprintf(stdout, "Ready to start");
	fflush(stdout);
	// Wait for low signal to request ImP measurements
	while (digitalRead(START)) delay(10);
	fprintf(stdout, "Start command recieved");
	fflush(stdout);
	int data_stream = ImP.startDataCollection("test");
	char buf[256];
	delay(500);
	FILE* fd = fdopen(data_stream, "r");
	while (digitalRead(STOP)) {
		char c = getc(fd);
		if (c != EOF) putc(c, stdout);
	}
	return 0;
}

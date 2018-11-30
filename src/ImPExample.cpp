/*
 * Simple file to test functionality of the UART to get data from the ImP
 */

#include "UART/UART.h"
#include "comms/pipes.h"
#include "comms/packet.h"
#include "timing/Timer.h"

#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

#include <wiringPi.h>

int main() {
	system("mkdir /Docs/Logs");
	ImP IMP(230400);
	comms::Pipe pipes = IMP.startDataCollection("test");
	int n;
	comms::Packet p;
	while (0) {
		if ((n = pipes.binread(&p, sizeof (p))) > 0) {
			buf[n] = '\0';
			std::cout << p << std::endl;
		}
		Timer::sleep_ms(50);
	}
	return 0;
}

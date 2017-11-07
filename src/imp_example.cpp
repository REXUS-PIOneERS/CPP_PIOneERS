#include "UART/UART.h"
#include "comms/pipes.h"
#include "timing/timer.h"
#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

int main() {
	ImP imp(230400);
	comms::Pipe pipes = imp.startDataCollection("test");
	char buf[256];
	int n;
	while (1) {
		n = pipes.binread(buf, 255);
		if (n > 0) {
			buf[n] = '\0';
			std::cout << buf << std::endl;
		}
		Timer::sleep_ms(10);
	}
}


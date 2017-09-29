#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>

#include "Ethernet/Ethernet.h"

int main() {

	Server server = Server(31415);
	int pipes[2];

	server.run(pipes);
	// Loop for sending and receiving data via pipes
	std::string msg = "This is a test message";
	char buf[256];
	while (1) {
		write(pipes[1], msg.c_str(), msg.length());
		read(pipes[0], buf, 256);
		printf(stdout, "%s", buf);
		delay(100);
	}
}

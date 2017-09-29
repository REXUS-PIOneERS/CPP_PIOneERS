#include <stdio.h>
#include <unistd.h>

#include "Ethernet/Ethernet.h"

int main() {

	Client client = Client(31415, "amisonpi.local");
	int pipes[2];

	client.run(pipes);
	// Loop for sending and receiving data via pipes
	std::string msg = "This is a test message\n";
	char* buf[256];
	std::string msg = "This is a test message";
	char buf[256];
	while (1) {
		write(pipes[1], msg.c_str(), msg.length());
		read(pipes[0], buf, 256);
		printf(stdout, "%s", buf);
		delay(100);
	}

	return 0;
}
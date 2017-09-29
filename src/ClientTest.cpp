#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>

#include "Ethernet/Ethernet.h"

int main() {

	Client client = Client(31415, "amisonpi.local");
	int pipes[2];
	printf("Starting client running\n");
	client.run(pipes);
	printf("Client is running\n");
	// Loop for sending and receiving data via pipes
	std::string msg = "This is a test message\n";
	char buf[256];
	while (1) {
		printf("Entering Loop\n");
		int n = write(pipes[1], msg.c_str(), 10);
		//int n = read(pipes[0], buf, 256);
		if (n != -1)
			printf("Message (%d): \n", n);
		delay(100);
	}

	return 0;
}

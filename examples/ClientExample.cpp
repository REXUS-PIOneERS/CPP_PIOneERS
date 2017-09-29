#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>

#include "Ethernet/Ethernet.h"

int main() {

	Client client = Client(31415, "amisonpi.local");
	int pipes[2];
	printf("Starting client running\n");
	int i = client.run(pipes);
	printf("Client is running %d\n", i);
	// Loop for sending and receiving data via pipes
	std::string msg = "This is a test message\n";
	char buf[256];
	while (1) {
		write(pipes[1], msg.c_str(), msg.length());
		int n = read(pipes[0], buf, 256);
		buf[n] = '\0';
		printf("Message (%d): %s \n", n, buf);
		delay(100);
	}

	return 0;
}

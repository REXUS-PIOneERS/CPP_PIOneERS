#include <stdio.h>
#include <unistd.h>

#include "Ethernet/Ethernet.h"

int main() {

	Server server = Server();
	int pipes[2];

	server.run(pipes);
	// Loop for sending and receiving data via pipes
	char* msg[50] = "This is a test message";
	FILE* file_stream = fdopen(pipes[1], "w");
	while (1)
		fputs(msg, file_stream);
}
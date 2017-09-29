#include <stdio.h>
#include <unistd.h>

#include "Ethernet/Ethernet.h"

int main() {

	Client client = Client();
	int pipes[2];

	client.run(pipes);
	// Loop for sending and receiving data via pipes
	char* msg[50] = "This is a test message\n";
	char* buf[256];
	FILE* read_stream = fdopen(pipes[1], "w");
	FILE* write_stream = fdopen(pipes[0], "r");
	while (1) {
		fputs(msg, file_stream);
		fgets(buf, 255, write_stream);
		fprintf(stdout, "%s", buf);
	}

	return 0;
}
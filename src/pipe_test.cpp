/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>

#include "pipes/pipes.h"

int main() {
	Pipe streams = Pipe();
	int pid;
	if ((pid = streams.Fork()) == 0) {
		/*
		 * This is the child process. It will continually poll the streams and
		 * echo any data received back to the parent process. If there is
		 * no data to read then this will be displayed.
		 */
		std::string msg;
		char buf[256];
		int i;
		while (1) {
			msg = streams.strread();
			if (!msg.empty()) {
				sprintf(buf, "%s (%d)", msg.c_str(), i);
				i = 0;
				streams.strwrite(buf);
				if (msg[0] == 'E')
					break;
			} else
				i++;
		}
	} else {
		// This is the parent process
		char buf[256];
		for (int i = 0; i < 10; i++) {
			delay(100);
			std::string msg = "TEST";
			sprintf(buf, "%s (%d)", msg.c_str(), i);
			streams.strwrite(msg);
			while (1) {
				msg = streams.strread();
				if (!msg.empty()) {
					fprintf(stdout, "%s\n", msg.c_str());
					break;
				}
			}
		}
		fprintf(stdout, "Closing parent pipe\n")
		streams.close();
		delay(10000);
		fprintf(stdout, "EXITING PROGRAM\n");
	}

	return 0;
}

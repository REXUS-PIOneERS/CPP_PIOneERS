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
		int i;
		while (1) {
			msg = streams.strread();
			if (!msg.empty()) {
				sprintf(msg, "%s (%d)", msg, i);
				i = 0;
				streams.strwrite(msg);
				if (msg[0] == 'E')
					break;
			} else
				i++;
		}
	} else {
		// This is the parent process
		for (int i = 0; i < 10; i++) {
			delay(100);
			std::string msg;
			sprintf(msg, "%s (%d)", msg, i);
			streams.strwrite(msg);
		}
	}

	return 0;
}
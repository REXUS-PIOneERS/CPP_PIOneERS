#include <stdlib.h>
#include <string>
#include "pipes/pipes.h"

#ifndef UART_H
#define UART_H

class UART {
	int uart_filestream;
	int m_pid;

public:

	UART() {
		setupUART();
	}

	void setupUART();
	int sendBytes(char *buf, size_t count);
	int getBytes(char buf[256]);
	Pipe startDataCollection(std::string filename);
	int stopDataCollection();
};


#endif /* UART_H */


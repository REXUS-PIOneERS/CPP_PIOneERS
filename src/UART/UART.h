#include <stdlib.h>
#include <string>

#ifndef UART_H
#define UART_H

class UART {
	int uart_filestream = -1;
	int m_pid;

public:

	UART() {
		setupUART();
	}

	void setupUART();
	int sendBytes(unsigned char *buf, size_t count);
	int getBytes(unsigned char buf[256]);
	int startDataCollection(std::string filename);
	int stopDataCollection();
};


#endif /* UART_H */


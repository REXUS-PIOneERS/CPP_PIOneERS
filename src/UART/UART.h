#ifndef UART_H
#define UART_H

class UART {
	int uart_filestream = -1;

public:

	UART() {
		setupUART();
	}

	void setupUART();
	int sendBytes(unsigned char *buf, size_t count);
	int getBytes(unsigned char buf[256]);
};


#endif /* UART_H */


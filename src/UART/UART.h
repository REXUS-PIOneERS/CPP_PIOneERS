/**
 * REXUS PIOneERS - Pi_1
 * UART.h
 * Purpose: Function definitions for the UART class
 *
 * @author David Amison
 * @version 2.2 12/10/2017
 */

#include <stdlib.h>
#include <string>
#include "pipes/pipes.h"

#ifndef UART_H
#define UART_H

class UART {
	Pipe m_pipes;
	int uart_filestream;
	int m_pid;

public:

	UART() {
		setupUART();
	}

	/**
	 * Basic setup for UART communication protocol
	 */
	void setupUART();

	/**
	 * Send bytes over UART connection.
	 *
	 * @param buf: Buffer containing the bytes to be sent
	 * @param n: Number of bytes to send
	 * @return The number of bytes read
	 */
	int sendBytes(const void *buf, int n);

	/**
	 * Get bytes over the UART connection
	 *
	 * @param buf: Buffer for receiving the bytes
	 * @param n: Maximum number of bytes to read
	 * @return The number of bytes read
	 */
	int getBytes(void *buf, int n);

	/**
	 * Collect and save data from the ImP. Returned pipe receives all data from
	 * the ImP as well.
	 *
	 * @param filename: Place to save data
	 * @return Pipe for sending and receiving data.
	 */
	Pipe startDataCollection(const std::string filename);

	/**
	 * Stop the process collecting data.
	 * @return 0 = success, otherwise = failure
	 */
	int stopDataCollection();
};


#endif /* UART_H */


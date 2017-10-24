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
#include <error.h>
#include "comms/pipes.h"

#ifndef UART_H
#define UART_H

class UART {
	comms::Pipe m_pipes;
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
	 * Collect and save data from the ImP. Returned pipe receives all data from
	 * the ImP as well.
	 *
	 * @param filename: Place to save data
	 * @return Pipe for sending and receiving data.
	 */
	comms::Pipe startDataCollection(const std::string filename);

	/**
	 * Stop the process collecting data.
	 * @return 0 = success, otherwise = failure
	 */
	int stopDataCollection();
};

class UARTException {
public:

	UARTException(std::string error) {
		m_error = error + " :" + std::strerror(errno);
	}

	const char * what() {
		return m_error.c_str();
	}

private:
	std::string m_error;
};

#endif /* UART_H */


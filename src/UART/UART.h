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
#include "comms/transceiver.h"

#include "logger/logger.h"

#ifndef UART_H
#define UART_H

class UART {
protected:
	int uart_filestream;

private:
	int m_baudrate;

public:

	UART(int baudrate) {
		m_baudrate = baudrate;
		setupUART();
	}

	/**
	 * Basic setup for UART communication protocol
	 */
	void setupUART();

	~UART();
};

class RXSM : public UART, public comms::Transceiver {
	Logger log;
	int _index = 0;

public:

	RXSM(int baudrate = 38400) : UART(baudrate), comms::Transceiver(uart_filestream), log("/Docs/Logs/RXSM") {
		log.start_log();
		log("INFO") << "Creating RXSM object";
		return;
	}

	int sendMsg(std::string &msg);

	int sendPacket(comms::Packet &p) {
		log("SENT") << p;
		return comms::Transceiver::sendPacket(p);
	}

	int recvPacket(comms::Packet &p) {
		int n = comms::Transceiver::recvPacket(p);
		if (n > 0)
			log("RECEIVED") << p;
		return n;
	}

	~RXSM() {
		log("INFO") << "Destroying RXSM object";
	}


};

class ImP : public UART {
	Logger log;
	comms::Pipe m_pipes;
	int m_pid;

public:

	ImP(int baudrate = 230400) : UART(baudrate), log("/Docs/Logs/ImP") {
		log.start_log();
		return;
	}

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


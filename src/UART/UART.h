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
	int _baudrate;

public:

	UART(int baudrate) {
		_baudrate = baudrate;
		setupUART();
	}

	/**
	 * Basic setup for UART communication protocol
	 */
	void setupUART();

	~UART();
};

class RXSM : public UART, public comms::Transceiver {
	Logger Log;
	int _index = 0;
	comms::Pipe _pipes;
	int _pid = 0;

public:

	RXSM(int baudrate = 38400) : UART(baudrate), comms::Transceiver(uart_filestream), Log("/Docs/Logs/RXSM") {
		Log.start_log();
		Log("INFO") << "Creating RXSM object";
		return;
	}

	int sendMsg(std::string msg);

	int sendPacket(comms::Packet &p);

	int recvPacket(comms::Packet &p);

	~RXSM() {
		Log("INFO") << "Destroying RXSM object";
	}

	void buffer();

	void end_buffer();

};

class ImP : public UART {
	Logger Log;
	comms::Pipe _pipes;
	int _pid;

public:

	ImP(int baudrate = 230400) : UART(baudrate), Log("/Docs/Logs/ImP") {
		Log.start_log();
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
		_error = error + " :" + std::strerror(errno);
	}

	const char * what() {
		return _error.c_str();
	}

private:
	std::string _error;
};

#endif /* UART_H */


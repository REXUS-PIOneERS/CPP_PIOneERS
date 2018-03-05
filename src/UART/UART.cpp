/**
 * REXUS PIOneERS - Pi_1
 * UART.cpp
 * Purpose: Function implementations for the UART class
 *
 * @author David Amison
 * @version 3.2 12/10/2017
 */
#include "UART.h"

#include <stdio.h>
#include <unistd.h>  //Used for UART
#include <fcntl.h>  //Used for UART
#include <termios.h> //Used for UART
// For multiprocessing
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>

#include <math.h>

#include "UART.h"
#include "comms/packet.h"
#include "comms/pipes.h"
#include "comms/transceiver.h"
#include "comms/protocol.h"

#include "timing/timer.h"

#include "logger/logger.h"
#include <error.h>

void UART::setupUART() {
	//Open the UART in non-blocking read/write mode
	uart_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY);
	if (uart_filestream == -1) {
		//throw UARTException("ERROR opening serial port");
		return;
	}
	speed_t f_baud;
	switch (_baudrate) {
		case 9600:
			f_baud = B9600;
			break;
		case 19200:
			f_baud = B19200;
			break;
		case 38400:
			f_baud = B38400;
			break;
		case 57600:
			f_baud = B57600;
			break;
		case 115200:
			f_baud = B115200;
			break;
		case 230400:
			f_baud = B230400;
			break;
		default:
			throw UARTException("ERROR baud rate not defined");
	}
	//Configure the UART
	struct termios options;
	tcgetattr(uart_filestream, &options);
	options.c_cflag = f_baud | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart_filestream, TCIFLUSH);
	tcsetattr(uart_filestream, TCSANOW, &options);
}

UART::~UART() {
	close(uart_filestream);
}

void RXSM::buffer() {
	try {
		_pipes = comms::Pipe();
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process
			Log.child_log();
			comms::Packet p;
			int n;
			while (1) {
				n = _pipes.binread(&p, sizeof (p));
				if (n > 0)
					sendPacket(p);

				n = recvPacket(p);
				if (n > 0)
					_pipes.binwrite(&p, sizeof (p));
				Timer::sleep_ms(1);
			}
		} else {
			// This is the parent process
			return;
		}
	} catch (comms::PipeException e) {
		Log("FATAL") << "Failed to read or write to pipe\n\t" << e.what();
		Log("INFO") << "Shutting down communication with RXSM";
		_pipes.close_pipes();
		exit(-1);
	} catch (...) {
		Log("FATAL") << "Unknown exception with RXSM\n\t" << std::strerror(errno);
		Log("INFO") << "Shutting down communication with RXSM";
		_pipes.close_pipes();
		exit(-2);
	}
}

void RXSM::end_buffer() {
	if (_pid)
		_pipes.close_pipes();
}

int RXSM::sendPacket(comms::Packet &p) {
	int n;
	if (_pid)
		n = _pipes.binwrite(&p, sizeof (p));
	else
		n = comms::Transceiver::sendPacket(&p);

	if (n > 0)
		Log("SENT") << p;
	else if (n < 0)
		Log("ERROR") << "Packet not sent\n\t" << std::strerror(errno);
	return n;
}

int RXSM::recvPacket(comms::Packet &p) {
	int n;
	if (_pid)
		n = _pipes.binread(&p, sizeof (p));
	else
		n = comms::Transceiver::recvPacket(&p);

	if (n > 0)
		Log("RECEIVED") << p;
	else if (n < 0)
		Log("ERROR") << "Problem getting data\n\t" << std::strerror(errno);
	return n;
}

int RXSM::sendMsg(std::string msg) {
	msg.insert(0, "Pi1: ");
	int n = msg.length();
	int mesg_num = ceil(n / (float) 15);
	char *buf = new char [17];
	int sent = 0;
	comms::Packet p;
	for (int i = 0; i < n; i += 16) {
		bzero(buf, 16);
		std::string data = msg.substr(i, 16);
		strcpy(buf, data.c_str());
		int msg_index = (_index++ << 8) | (--mesg_num);
		comms::Protocol::pack(p, ID_MSG1, msg_index, buf);
		sent += sendPacket(p);
	}
	return sent;
}

bool RXSM::status() {
	int status_check;
	if (_pid) {
		pid_t result = waitpid(_pid, &status_check, WNOHANG);
		if (result == 0)
			return true;
		else if (result == _pid)
			return false;
		else {
			Log("ERROR") << "Problem with status check\n\t" << std::strerror(errno);
			return false;
		}
	} else
		return false;
}

comms::Pipe ImP::startDataCollection(const std::string filename) {
	/*
	 * Sends request to the ImP to begin sending data. Returns the file stream
	 * to the main program and continually writes the data to this stream.
	 */
	Log("INFO") << "Starting ImP and IMU data collection";
	try {
		_pipes = comms::Pipe();
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process
			Log.child_log();
			// Infinite loop for data collection
			comms::Transceiver ImP_comms(uart_filestream);
			// Send initial start command
			ImP_comms.sendBytes("C", 1);
			Log("DATA (SENT") << "C";
			std::string measurement_start = Timer::str_datetime();
			Timer m_tmr; // Gives timing of all measurements
			int32_t m_time = 0;
			int err;
			for (int j = 0;; j++) {
				std::ofstream outf;
				std::stringstream unique_file;
				unique_file << filename << "_" << measurement_start << "_"
						<< std::setfill('0') << std::setw(4) << j << ".txt";
				Log("INFO") << "Starting new data file \"" << unique_file.str() << "\"";
				outf.open(unique_file.str());
				// Take five measurements then change the file
				int intv = 200;
				for (int i = 0; i < 5; i++) {
					Timer tmr;
					char buf[256];
					int buf_ind = 0;
					while ((buf_ind < 255) && (tmr.elapsed() < 2*intv)) {
						if (ImP_comms.recvBytes(buf + buf_ind, 1)) {
							if (buf[buf_ind-1] == 0)
								break;
							buf_ind++;
						} else {
							Timer::sleep_ms(1);
						}
					}
					int stuffed = buf[0];
					int carry;
					while ((stuffed < 255) && (carry = buf[stuffed]) != 0) {
						buf[stuffed] = 0;
						stuffed += carry;
					}
					for (int k = 0; k < buf_ind; k++) {
						outf << (int)buf[k] << " ";
					}
					Log("INFO") << "Recevied primary data";
					comms::Packet p1;
					comms::Packet p2;
					comms::Protocol::pack(p1, ID_DATA3, i+5*j, buf);
					comms::Protocol::pack(p2, ID_DATA4, i+5*j, (buf + 12));
					Log("DATA(ImP)") << p1;
					Log("DATA(ImP)") << p2;
					_pipes.binwrite(&p1, sizeof(comms::Packet));
					_pipes.binwrite(&p2, sizeof(comms::Packet));
					Log("INFO") << "Data sent to main process";

					//Now handle all the other numbers coming in
					int total = 0;
					comms::byte1_t char_buf = 'a';
					while (tmr.elapsed() < 2*intv) {
						if (ImP_comms.recvBytes(&char_buf, 1)) {
							outf << (int)char_buf << " ";
							total++;
						} else {
							Timer::sleep_ms(50);
							Log("INFO") << "No data-" << tmr.elapsed();
						}
						if (char_buf == 0) {
							outf << std::endl;
							break;
						}
					}
					Log("INFO") << "Received secondary data (" << total << ")";
					ImP_comms.sendBytes("N",1);
					while (tmr.elapsed() < intv)
						Timer::sleep_ms(1);
				}
				outf.close();
			}
		} else {
			return _pipes;
		}
	} catch (int e) {
		Log("FATAL") << "Unable to read/write to pipes(" << e << ")\n\t" << std::strerror(errno);
		Log("INFO") << "Ending the process";
		_pipes.close_pipes();
		close(uart_filestream);
		exit(e);
	} catch (...) {
		Log("FATAL") << "Unexpected error with ImP\n\t" << std::strerror(errno);
		Log("INFO") << "Emding the process";
		_pipes.close_pipes();
		close(uart_filestream);
		exit(-2);
	}
}

bool ImP::status() {
	int status_check;
	if (_pid) {
		pid_t result = waitpid(_pid, &status_check, WNOHANG);
		if (result == 0)
			return true;
		else if (result == _pid)
			return false;
		else {
			Log("ERROR") << "Problem with status check\n\t" << std::strerror(errno);
			return false;
		}
	} else
		return false;
}

int ImP::stopDataCollection() {
	if (_pid) {
		Log("INFO") << "Stopping ImP process (ID:" << _pid << ")";
		bool died = false;
		for (int i = 0; !died && i < 5; i++) {
			int status = 0;
			kill(_pid, SIGTERM);
			sleep(1);
			if (waitpid(_pid, &status, WNOHANG) == _pid) died = true;
		}
		if (died) {
			Log("INFO") << "ImP process terminated by sending SIGTERM signal";
		} else {
			Log("ERROR") << "SIGTERM signal failed, sending SIGKILL";
			kill(_pid, SIGKILL);
		}
		_pid = 0;
		_pipes.close_pipes();
	}
	return 0;
}

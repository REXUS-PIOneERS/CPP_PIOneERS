/**
 * REXUS PIOneERS - Pi_1
 * Ethernet.cpp
 * Purpose: Implementation of functions for the Server and Client classes for
 *		ethernet communication
 *
 * @author David Amison
 * @version 2.0 12/10/2017
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream> //for std::ofstream
#include <iostream>  //for std::endl

#include <string>
#include <fcntl.h>

#include "Ethernet.h"
#include "comms/pipes.h"
#include "comms/transceiver.h"
#include "comms/packet.h"

#include "timing/timer.h"
#include "logger/logger.h"
#include <error.h>
#include <sstream>

#include <poll.h>

int Server::status() {
	if (!_pid)
		return -1;
	// Checks a file descriptor is ready for reading
	int rtn = 0;
	struct pollfd fds[3];
	int timeout = 0;
	// Read end of Pipe
	fds[0].fd = _pipes.getReadfd();
	fds[0].events = POLLIN;
	// Write end of Pipe
	fds[1].fd = _pipes.getWritefd();
	fds[1].events = POLLOUT;
	// Ethernet file descriptor
	fds[2].fd = _newsockfd;
	fds[2].events = POLLIN | POLLOUT;
	// Check if the file descriptor is ready for reading
	if (poll(fds, 1, timeout)) {
		if (!(fds[0].revents & POLLIN))
			rtn &= 0x01;
		if (!(fds[1].revents & POLLOUT))
			rtn &= 0x02;
		if (!(fds[2].revents & (POLLIN | POLLOUT)))
			rtn &= 0x04;
	}
	return rtn;
}

// Functions for setting up as a server

int Server::setup() {
	Log("INFO") << "Setting up server to communicate on port no. " << _port;
	// Open the socket for Ethernet connection
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0) {
		Log("ERROE") << "Server failed to open socket";
		throw EthernetException("ERROR: Server Failed to Open Socket");
	}
	bzero((char *) &_serv_addr, sizeof (_serv_addr));
	// Setup details for the server address
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_addr.s_addr = INADDR_ANY;
	_serv_addr.sin_port = _port;
	// Bind the socket to given address and port number
	if (bind(_sockfd, (struct sockaddr *) &_serv_addr, sizeof (_serv_addr)) < 0) {
		Log("ERROR") << "Failed to bind server";
		throw EthernetException("ERROR: On binding server");
	}
	// Sets backlog queue to 5 connections and allows socket to listen
	listen(_sockfd, 5);
	_clilen = sizeof (_cli_addr);
	Log("INFO") << "Server setup successful";
	return 0;
}

Server::~Server() {
	Log("INFO") << "Destroying Server";
	Log.stop_log();
	close(_newsockfd);
	close(_sockfd);
}

// Functions for setting up as a client

int Client::setup() {
	Log("INFO") << "Setting up client to communicate with " << _host_name <<
			" on port no. " << _port;
	// Open the socket
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sockfd < 0) {
		Log("ERROR") << "Client failed to open socket";
		throw EthernetException("Client failed to open socket");
	}
	// Look for server host by given name
	_server = gethostbyname(_host_name.c_str());
	if (_server == NULL) {
		Log("ERROR") << "Cannot find host \"" << _host_name << "\"";
		throw EthernetException("No such host");
	}
	bzero((char *) &_serv_addr, sizeof (_serv_addr));
	_serv_addr.sin_family = AF_INET;
	bcopy((char *) _server->h_addr,
			(char *) &_serv_addr.sin_addr.s_addr,
			_server->h_length);
	_serv_addr.sin_port = _port;
	Log("INFO") << "Client setup successful";
	return 0;
}

int Client::open_connection() {
	Log("INFO") << "Opening connection to server";
	if (connect(_sockfd, (struct sockaddr *) &_serv_addr, sizeof (_serv_addr)) < 0) {
		Log("ERROR") << "Request to connect to server failed";
		throw EthernetException("Failed to connect to server"); // Failed to connect!
	}
	_connected = true;
	return 0;
}

int Client::close_connection() {
	Log("INFO") << "Ending connection with server and closing process";
	if (_sockfd) {
		close(_sockfd);
		_sockfd = 0;
		_pipes.close_pipes();
	}
	_connected = false;
}

Client::~Client() {
	Log("INFO") << "Destroying Client";
	close_connection();
	Log.stop_log();
}

/*
 * Specific functions for the PIOneERS implementation. The client and server
 * both run as a separate process with a pipe for passing data back and forth.
 * The client regularly checks the pipe for data to be sent and does so.
 * Whenever the server receives a request it also checks it's own pipe for data
 * to be transferred to the client and does so.
 *
 * General format of communication:
 * Client initiates communication and sends first data packet
 * Server responds by acknowledging receipt of the packet
 * Client continues to send packets while it has some and the server responds
 *		accordingly
 * Client informs the server it has finished sending packets.
 * Server then sends any packets it has to the client in a similar process.
 * Server informs the client it is finished and both server and client end
 *		the connection
 */

comms::Pipe Client::run(std::string filename) {
	/*
	 * Generate a pipe for transferring the data, fork the process and send
	 * back the required pipes.
	 * The child process stays open periodically checking the pipe for data
	 * packets and sending these to the server as well as receiving packets
	 * in return.
	 */
	Log("INFO") << "Starting data sharing with server";
	try {
		_pipes = comms::Pipe();
		open_connection();
		Log("INFO") << "Forking processes";
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process.
			Log.child_log();
			comms::Transceiver eth_comms(_sockfd);
			std::ofstream outf;
			outf.open(filename);
			comms::Packet p;
			while (1) {
				// Exchange packets (no analysis of contents)
				if (_pipes.binread(&p, sizeof (p)) > 0) {
					Log("DATA (CLIENT)") << p;
					eth_comms.sendPacket(&p);
				}

				if (eth_comms.recvPacket(&p) > 0) {
					Log("DATA (SERVER)") << p;
					_pipes.binwrite(&p, sizeof (p));
					outf << p << std::endl;
				}
				Timer::sleep_ms(10);
			}
			outf.close();
			_pipes.close_pipes();
			exit(0);
		} else {
			// Assign the pipes for the main process and close the un-needed ones
			return _pipes;
		}
	} catch (comms::PipeException e) {
		Log("FATAL") << "Unable to read/write to pipes\n\t\"" << e.what() << "\"";
		_pipes.close_pipes();
		exit(-1);
	} catch (EthernetException e) {
		Log("FATAL") << "Problem with communication\n\t\"" << e.what() << "\"";
		fprintf(stdout, "Ethernet- %s\n", e.what());
		_pipes.close_pipes();
		throw e;
	} catch (...) {
		Log("FATAL") << "Unexpected error with client\n\t\""
				<< std::strerror(errno) << "\"";
		_pipes.close_pipes();
		exit(-3);
	}
}

comms::Pipe Server::run(std::string filename) {
	// Fork a process to handle server stuff
	Log("INFO") << "Starting data sharing with client";
	try {
		Log("INFO") << "Waiting for client connection";
		_newsockfd = accept(_sockfd, (struct sockaddr*) & _cli_addr, &_clilen);
		if (_newsockfd < 0)
			throw -3;
		Log("INFO") << "Client has established connection";

		Log("INFO") << "Forking processes";
		_pipes = comms::Pipe();
		if ((_pid = _pipes.Fork()) == 0) {
			// This is the child process that handles all the requests
			Log.child_log();
			comms::Transceiver eth_comms(_newsockfd);
			std::ofstream outf;
			outf.open(filename);
			comms::Packet p;
			int n;
			while (1) {
				// Get packet from client and send to pipe
				n = eth_comms.recvPacket(&p);
				if (n < 0)
					throw -2;
				else if (n > 0) {
					Log("RECEIVED") << p;
					outf << p << std::endl;
					if (_pipes.binwrite(&p, sizeof (p)) < 0)
						throw -1;
				}

				// Get packet from pipe and send to server
				n = _pipes.binread(&p, sizeof (p));
				if (n < 0)
					throw -1;
				else if (n > 0) {
					Log("SENDING") << p;
					if (eth_comms.sendPacket(&p) < 0)
						throw -2;
				}
				Timer::sleep_ms(10);
			}
		} else {
			// This is the main parent process
			return _pipes;
		}
	} catch (int e) {
		switch (e) {
			case -1:
				// Problem with the pipes
				Log("ERROR") << "Problem with the pipes\n\t" << std::strerror(errno);
				close(_newsockfd);
				_pipes.close_pipes();
				break;
			case -2:
				// Problem with the Ethernet
				Log("ERROR") << "Problem with the Ethernet\n\t" << std::strerror(errno);
				close(_newsockfd);
				_pipes.close_pipes();
				break;
			case -3:
				// Problem with accept
				Log("ERROR") << "Error on accept\n\t" << std::strerror(errno);
				return _pipes;
		}
		exit(e);
	}
}


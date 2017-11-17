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
#include <sstream> //for std::stringstream
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
	m_clilen = sizeof (_cli_addr);
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
	return 0;
}

void Client::close_connection() {
	Log("INFO") << "Ending connection with server and closing process";
	if (_sockfd) {
		close(_sockfd);
		_pipes.close_pipes();
	}
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

void Raspi1::share_data() {
	while (1) {
		try {
			setup();
			open_connection();
			_is_running = true;
			comms::Transceiver eth_comms(_sockfd);
			std::ofstream outf;
			std::stringstream outf_name;
			outf_name << _filename << "_" << Timer::str_datetime();
			outf.open(outf_name.str());
			comms::Packet p;
			while (1) {
				int n;
				n = eth_comms.recvPacket(&p);
				if (n < 0) throw EthernetException("Error receiving packet");
				else if (n > 0) {
					Log("DATA (SERVER)") << p;
					outf << p << std::endl;
					n = _pipes.binwrite(&p, sizeof (comms::Packet));
					if (n < 0) throw n;
				}

				n = _pipes.binread(&p, sizeof (comms::Packet));
				if (n < 0) throw n;
				else if (n > 0) {
					Log("DATA (CLIENT)") << p;
					n = eth_comms.sendPacket(&p);
					if (n < 0) throw EthernetException("Error sending packet");
				}
				Timer::sleep_ms(1);
			}
		} catch (int e) {
			_is_running = false;
			switch (e) {
				case -1: // Process not forked correctly
					Log("ERROR") << "Problem with ethernet\n\t" << std::strerror(errno);
					break;
				case -2: // Pipe unavailable
				case -3: //
					Log("ERROR") << "Problem with read/write to pipes\n\t" << std::strerror(errno);
					close(_sockfd);
					_pipes.close_pipes();
					exit(0); // This is expected when we want to end the process
				default:
					Log("ERROR") << "Unexpected error code: " << e;
					close(_sockfd);
					_pipes.close_pipes();
					exit(-1);
			}
			_pipes.close_pipes();
		} catch (EthernetException e) {
			_is_running = false;
			Log("ERROR") << "Problem with communication\n\t" << e.what();
			_pipes.close_pipes();
			Log("INFO") << "Trying to reconnect";
		} catch (...) {
			_is_running = false;
			Log("FATAL") << "Unexpected error with client\n\t" << std::strerror(errno);
			_pipes.close_pipes();
			exit(-2);
		}
	}
}

void Raspi1::run(std::string filename) {
	/*
	 * Generate a pipe for transferring the data, fork the process and send
	 * back the required pipes.
	 * The child process stays open periodically checking the pipe for data
	 * packets and sending these to the server as well as receiving packets
	 * in return.
	 */
	_filename = filename;
	Log("INFO") << "Starting data sharing with server";
	_pipes = comms::Pipe();
	Log("INFO") << "Forking processes";
	if ((_pid = _pipes.Fork()) == 0) {
		// This is the child process.
		Log.child_log();
		share_data();
	} else {
		// Assign the pipes for the main process and close the un-needed ones
		return _pipes;
	}
}

void Raspi2::share_data() {
	while (1) {
		try {
			setup();
			Log("INFO") << "Waiting for client connection";
			_newsockfd = accept(_sockfd, (struct sockaddr*) & _cli_addr, &m_clilen);
			if (_newsockfd < 0) {
				Log("ERROR") << "Problem waiting for client connection";
				throw EthernetException("Error on accept");
			}
			_is_running = true;
			Log("INFO") << "Client has established connection";

			comms::Transceiver eth_comms(_newsockfd);
			std::ofstream outf;
			std::stringstream outf_name;
			outf_name << _filename << "_" << Timer::str_datetime();
			outf.open(outf_name.str());
			comms::Packet p;
			while (1) {
				int n;
				n = eth_comms.recvPacket(&p);
				if (n < 0) throw EthernetException("Error receiving packet");
				else if (n > 0) {
					Log("DATA (CLIENT)") << p;
					outf << p << std::endl;
					n = _pipes.binwrite(&p, sizeof (comms::Packet));
					if (n < 0) throw n;
				}

				n = _pipes.binread(&p, sizeof (comms::Packet));
				if (n < 0) throw n;
				else if (n > 0) {
					Log("DATA (SERVER)") << p;
					n = eth_comms.sendPacket(&p);
					if (n < 0) throw EthernetException("Error sending packet");
				}
				Timer::sleep_ms(1);
			}
		} catch (int e) {
			_is_running = false;
			switch (e) {
				case -1: // Process not forked correctly
					Log("ERROR") << "Problem with ethernet\n\t" << std::strerror(errno);
					break;
				case -2: // Pipe unavailable
				case -3: //
					Log("ERROR") << "Problem with read/write to pipes\n\t" << std::strerror(errno);
					close(_newsockfd);
					close(_sockfd);
					_pipes.close_pipes();
					exit(0); // This is expected when we want to end the process
				default:
					Log("ERROR") << "Unexpected error code: " << e;
					close(_newsockfd);
					close(_sockfd);
					_pipes.close_pipes();
					exit(-1);
			}
			close(_newsockfd);
			_pipes.close_pipes();
		} catch (EthernetException e) {
			_is_running = false;
			Log("FATAL") << "Problem with Ethernet communication\n\t" << e.what();
			close(_newsockfd);
			_pipes.close_pipes();
			// Allow program to try and reconnect
		} catch (...) {
			_is_running = false;
			Log("FATAL") << "Unexpected error with server\n\t" << std::strerror(errno);
			close(_newsockfd);
			close(_sockfd);
			_pipes.close_pipes();
			exit(-3);
		}
	}
}

void Raspi2::run(std::string filename) {
	// Fork a process to handle server stuff
	_filename = filename;
	Log("INFO") << "Starting data sharing with client";
	Log("INFO") << "Forking processes";
	if ((_pid = _pipes.Fork()) == 0) {
		// This is the child process that handles all the requests
		Log.child_log();
		share_data();
	} else {
		// This is the main parent process
		return;
	}
}

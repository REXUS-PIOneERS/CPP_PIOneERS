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

// Functions for setting up as a server

int Server::setup() {
	log("INFO") << "Setting up server to communicate on port no. " << m_port;
	// Open the socket for Ethernet connection
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockfd < 0) {
		log("ERROE") << "Server failed to open socket";
		throw EthernetException("ERROR: Server Failed to Open Socket");
	}
	bzero((char *) &m_serv_addr, sizeof (m_serv_addr));
	// Setup details for the server address
	m_serv_addr.sin_family = AF_INET;
	m_serv_addr.sin_addr.s_addr = INADDR_ANY;
	m_serv_addr.sin_port = m_port;
	// Bind the socket to given address and port number
	if (bind(m_sockfd, (struct sockaddr *) &m_serv_addr, sizeof (m_serv_addr)) < 0) {
		log("ERROR") << "Failed to bind server";
		throw EthernetException("ERROR: On binding server");
	}
	// Sets backlog queue to 5 connections and allows socket to listen
	listen(m_sockfd, 5);
	m_clilen = sizeof (m_cli_addr);
	log("INFO") << "Server setup successful";
	return 0;
}

Server::~Server() {
	log("INFO") << "Destroying Server";
	log.stop_log();
	close(m_newsockfd);
	close(m_sockfd);
}

// Functions for setting up as a client

int Client::setup() {
	log("INFO") << "Setting up client to communicate with " << m_host_name <<
			" on port no. " << m_port;
	// Open the socket
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockfd < 0) {
		log("ERROR") << "Client failed to open socket";
		throw EthernetException("Client failed to open socket");
	}
	// Look for server host by given name
	m_server = gethostbyname(m_host_name.c_str());
	if (m_server == NULL) {
		log("ERROR") << "Cannot find host \"" << m_host_name << "\"";
		throw EthernetException("No such host");
	}
	bzero((char *) &m_serv_addr, sizeof (m_serv_addr));
	m_serv_addr.sin_family = AF_INET;
	bcopy((char *) m_server->h_addr,
			(char *) &m_serv_addr.sin_addr.s_addr,
			m_server->h_length);
	m_serv_addr.sin_port = m_port;
	log("INFO") << "Client setup successful";
	return 0;
}

int Client::open_connection() {
	log("INFO") << "Opening connection to server";
	if (connect(m_sockfd, (struct sockaddr *) &m_serv_addr, sizeof (m_serv_addr)) < 0) {
		log("ERROR") << "Request to connect to server failed";
		throw EthernetException("Failed to connect to server"); // Failed to connect!
	}
	return 0;
}

int Client::close_connection() {
	log("INFO") << "Ending connection with server and closing process";
	if (m_sockfd) {
		close(m_sockfd);
		m_pipes.close_pipes();
	}
}

Client::~Client() {
	log("INFO") << "Destroying Client";
	close_connection();
	log.stop_log();
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
	log("INFO") << "Starting data sharing with server";
	try {
		m_pipes = comms::Pipe();
		setup();
		open_connection();
		log("INFO") << "Forking processes";
		if ((m_pid = m_pipes.Fork()) == 0) {
			// This is the child process.
			comms::Transceiver eth_comms(m_sockfd);
			std::ofstream outf;
			outf.open(filename);
			comms::Packet p;
			while (1) {
				// Exchange packets (no analysis of contents)
				if (m_pipes.binread(&p, sizeof (p)) > 0) {
					log("DATA (CLIENT)") << p;
					eth_comms.sendPacket(&p);
				}

				if (eth_comms.recvPacket(&p) > 0) {
					log("DATA (SERVER)") << p;
					m_pipes.binwrite(&p, sizeof (p));
					outf << p << std::endl;
				}
				Timer::sleep_ms(10);
			}
			outf.close();
			m_pipes.close_pipes();
			exit(0);
		} else {
			// Assign the pipes for the main process and close the un-needed ones
			return m_pipes;
		}
	} catch (comms::PipeException e) {
		log("FATAL") << "Unable to read/write to pipes\n\t\"" << e.what() << "\"";
		m_pipes.close_pipes();
		exit(-1);
	} catch (EthernetException e) {
		log("FATAL") << "Problem with communication\n\t\"" << e.what() << "\"";
		fprintf(stdout, "Ethernet- %s\n", e.what());
		m_pipes.close_pipes();
		throw e;
	} catch (...) {
		log("FATAL") << "Unexpected error with client\n\t\""
				<< std::strerror(errno) << "\"";
		m_pipes.close_pipes();
		exit(-3);
	}
}

comms::Pipe Server::run(std::string filename) {
	// Fork a process to handle server stuff
	log("INFO") << "Starting data sharing with client";
	try {
		m_pipes = comms::Pipe();
		log("INFO") << "Waiting for client connection";
		m_newsockfd = accept(m_sockfd, (struct sockaddr*) & m_cli_addr, &m_clilen);
		if (m_newsockfd < 0) {
			log("FATAL") << "Error waiting for client connection";
			throw EthernetException("Error on accept");
		}
		log("INFO") << "Client has established connection";
		log("INFO") << "Forking processes";
		if ((m_pid = m_pipes.Fork()) == 0) {
			comms::Transceiver eth_comms(m_newsockfd);
			// This is the child process that handles all the requests
			std::ofstream outf;
			outf.open(filename);
			comms::Packet p;
			while (1) {
				if (eth_comms.recvPacket(&p) > 0) {
					log("DATA (CLIENT)") << p;
					outf << p << std::endl;
					m_pipes.binwrite(&p, sizeof (comms::Packet));
				}
				if (m_pipes.binread(&p, sizeof (comms::Packet)) > 0) {
					log("DATA (SERVER)") << p;
					eth_comms.sendPacket(&p);
				}
				Timer::sleep_ms(10);

			}
			outf.close();
			close(m_newsockfd);
			close(m_sockfd);
			m_pipes.close_pipes();
			exit(0);
		} else {
			// This is the main parent process
			return m_pipes;
		}
	} catch (comms::PipeException e) {
		// Ignore it and exit gracefully
		log("FATAL") << "Problem reading/writing to pipes\n\t\"" << e.what()
				<< "\"";
		close(m_newsockfd);
		close(m_sockfd);
		m_pipes.close_pipes();
		exit(-1);
	} catch (EthernetException e) {
		log("FATAL") << "Problem with Ethernet communication\n\t\"" << e.what()
				<< "\"";
		close(m_newsockfd);
		close(m_sockfd);
		m_pipes.close_pipes();
		throw e;
	} catch (...) {
		log("FATAL") << "Unexpected error with server\n\t\""
				<< std::strerror(errno) << "\"";
		close(m_newsockfd);
		close(m_sockfd);
		m_pipes.close_pipes();
		exit(-3);
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <fcntl.h>

#include "Ethernet.h"

void error(const char *msg) {
	perror(msg);
	exit(1);
}

// Functions for setting up as a server

Server::Server(int port) {
	m_port = port;

	setup();
}

int Server::setup() {
	// Open the socket for Ethernet connection
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &m_serv_addr, sizeof (m_serv_addr));
	// Setup details for the server address
	m_serv_addr.sin_family = AF_INET;
	m_serv_addr.sin_addr.s_addr = INADDR_ANY;
	m_serv_addr.sin_port = m_port;
	// Bind the socket to given address and port number
	if (bind(m_sockfd, (struct sockaddr *) &m_serv_addr,
			sizeof (m_serv_addr)) < 0)
		error("ERROR on binding");
	// Sets backlog queue to 5 connections and allows socket to listen
	listen(m_sockfd, 5);
	m_clilen = sizeof (m_cli_addr);
}

std::string Server::receive_packet() {
	// Receive a packet of data from the client and ackowledge receipt
	char buf[256];
	int n;
	n = read(m_newsockfd, buf, 255);
	if (n <= 0)
		return "";
	buf[n] = '\0';
	std::string packet(buf);
	return packet;
}

int Server::send_packet(std::string packet) {
	/*
	 * Sends a data packet to the server. General format is:
	 * [SYNC] [MSGID] [MSGLEN] [DATA] [CRC]
	 * Data uses consistent overhead byte stuffing
	 */
	char buf[256];
	int n = write(m_newsockfd, packet.c_str(), packet.length());
	// Check we managed to send the data
	return (n > 0) ? 0 : -1;
}

Server::~Server() {
	close(m_newsockfd);
	close(m_sockfd);
}




// Functions for setting up as a client

Client::Client(int port, std::string host_name) {
	m_host_name = host_name;
	m_port = port;
	setup();
}

int Client::setup() {
	// Open the socket
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sockfd < 0)
		error("ERROR: failed to open socket");
	// Look for server host by given name
	m_server = gethostbyname(m_host_name.c_str());
	if (m_server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &m_serv_addr, sizeof (m_serv_addr));
	m_serv_addr.sin_family = AF_INET;
	bcopy((char *) m_server->h_addr,
			(char *) &m_serv_addr.sin_addr.s_addr,
			m_server->h_length);
	m_serv_addr.sin_port = m_port;

	return 0;
}

int Client::open_connection() {
	if (connect(m_sockfd, (struct sockaddr *) &m_serv_addr, sizeof (m_serv_addr)) < 0)
		return -1; // Failed to connect!
	return 0;
}

int Client::send_packet(std::string packet) {
	/*
	 * Sends a data packet to the server. General format is:
	 * [SYNC] [MSGID] [MSGLEN] [DATA] [CRC]
	 * Data uses consistent overhead byte stuffing
	 */
	char buf[256];
	bzero(buf, 256);
	int n = write(m_sockfd, packet.c_str(), packet.length());
	// Check receipt has been acknowledged
	return (n > 0) ? 0 : -1;
}

std::string Client::receive_packet() {
	// Receives a data packet from the server
	char buf[256];
	int n = read(m_sockfd, buf, 255);
	if (n <= 0)
		return "";
	buf[n] = '\0';
	std::string packet(buf);
	return packet;
}

int Client::close_connection() {
	if (m_sockfd) {
		send_packet("E");
	}
}

Client::~Client() {
	if (m_sockfd) {
		send_packet("E");
		close(m_sockfd);
	}
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

int Client::run(int pipes[2]) {
	/*
	 * Generate a pipe for transferring the data, fork the process and send
	 * back the required pipes.
	 * The child process stays open periodically checking the pipe for data
	 * packets and sending these to the server as well as receiving packets
	 * in return.
	 */
	int write_pipe[2]; // This pipe sends data back to the main process
	int read_pipe[2]; // This pipe receives data from the main process
	pipe(write_pipe);
	pipe(read_pipe);
	if (open_connection() == -1)
		return -1;
	if ((m_pid = fork()) == 0) {
		// This is the child process.
		close(write_pipe[0]);
		close(read_pipe[1]);
		// Loop for sending and receiving data
		std::ofstream outf;
		outf.open("Docs/Data/Pi2/backup.txt");
		while (1) {
			char buf[256];
			bzero(buf, 256);
			// Send any data we have
			int n = read(read_pipe[0], buf, 255);
			if (n <= 0)
				continue;
			buf[n] = '\0';
			std::string packet_send(buf);
			if (send_packet(packet_send) != 0)
				continue; // TODO handle the error
			// Check for exit message
			if (packet_send[0] == 'E')
				break;
			// Loop for receiving packets
			std::string packet_recv = receive_packet();
			if (!packet_recv.empty()) {
				outf << packet_recv;
				//write(write_pipe[1], packet_recv.c_str(), packet_recv.length());
			}
		}
		outf.close();

	} else {
		// Assign the pipes for the main process and close the un-needed ones
		close(write_pipe[1]);
		close(read_pipe[0]);
		pipes[0] = write_pipe[0];
		pipes[1] = read_pipe[1];
		return 0;
	}
	return 0;
}

int Server::run(int *pipes) {
	// Fork a process to handle server stuff
	int read_pipe[2];
	int write_pipe[2];
	pipe(read_pipe);
	pipe(write_pipe);

	while (1) {
		printf("Waiting for client connection...\n");
		m_newsockfd = accept(m_sockfd, (struct sockaddr*) & m_cli_addr, &m_clilen);
		if (m_newsockfd < 0) error("ERROR: on accept");
		printf("Connection established with a new client...\n"
				"Beginning data sharing...\n");
		if ((m_pid = fork()) == 0) {
			// This is the child process that handles all the requests
			close(read_pipe[1]);
			close(write_pipe[0]);
			// Loop for receiving data
			std::ofstream outf;
			outf.open("Docs/Data/Pi1/backup.txt");
			char buf[256];
			while (1) {
				// Try to get data from the Client
				std::string packet_recv = receive_packet();
				if (!packet_recv.empty()) {
					outf << packet_recv;
					//write(write_pipe[1], packet_recv.c_str(), packet_recv.length());
					if (packet_recv[0] == 'E')
						break;
				}
				// Try to send data to the Client
				int n = read(read_pipe[0], buf, 255);
				if (n <= 0)
					continue;
				buf[n] = '\0';
				std::string packet_send(buf);
				if (send_packet(packet_send) != 0)
					continue; // TODO Handling this error
			}
			outf.close();
		} else {
			// This is the main parent process
			close(read_pipe[0]);
			close(write_pipe[1]);
			pipes[0] = write_pipe[0];
			pipes[1] = read_pipe[1];
			return 0;
		}
		close(m_newsockfd);
	}
	return -1; // Something must have gone wrong because this shouldn't happen
}

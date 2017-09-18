#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Ethernet.h"

void error(const char *msg) {
	perror(msg);
	exit(1);
}

// Functions for setting up as a server

Server::Server(int port) {
	m_port = port;
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
	// Wait for a request from the client (Infinite Loop)
	while (1) {
		printf("Waiting for new client...\n");
		m_newsockfd = accept(m_sockfd, (struct sockaddr *) &m_cli_addr, &m_clilen);
		if (m_newsockfd < 0) error("ERROR: On accept");
		printf("New connection established with client...\n");
		// Loop for back and forth communication
		while (1) {
			char buffer[256];
			bzero(buffer, 256);
			int n = read(m_newsockfd, buffer, 255);
			buffer[n] = NULL;
			if (n < 0) error("ERROR: Reading from socket");
			else {
				if (strncmp(buffer, "E", 1) == 0) break;
				fprintf(stdout, "Message Received: % s\n", buffer);
				n = write(m_newsockfd, "RECEIVED", 8);
				if (n < 0) error("ERROR writing to socket");
			}
		}
		close(m_newsockfd);
	}
	close(m_sockfd);
	return 0;
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
	m_server = gethostbyname(m_host_name.s_str());
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

std::string Client::send_packet(std::string packet) {
	/*
	 * Sends a data packet to the server. General format is:
	 * [SYNC] [MSGID] [MSGLEN] [DATA] [CRC]
	 * Data uses consistent overhead byte stuffing
	 */
	int n = 0;
	char buffer[256];
	bzero(buffer, 256);
	int attempts = 0;
	// Try to send the packet (maximum of 3 attempts)
	while (n <= 0 && attempts++ < 3)
		n = write(m_sockfd, packet, strlen(packet));
	// Get a response from the server
	n = read(m_sockfd, buffer, 255);
	buffer[n] = 0; // Add null character for termination of string
	// Convert char* to std::string then return
	std::string rtn(buffer);
	return rtn;
}

int Client::close_connection() {
	send_packet("EXIT"); // When this command is received server terminates connection
	close(m_sockfd);
}

Client::~Client() {
	if (m_sockfd) {
		send_packet("EXIT");
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
	int sendPipe[2]; // This pipe sends data back to the main process
	int recvPipe[2]; // This pipe receives data from the main process
	open_connection();
	if (pipe(sendPipe)) {
		fprintf(stderr, "Client failed to make sendPipe");
		return -1;
	}
	if (pipe(recvPipe)) {
		fprintf(stderr, "Client failed to make recvPipe");
		return -1;
	}
	if ((m_pid = fork()) == 0) {
		// This is the child process. First close the pipes we don't need
		close(sendPipe[0]);
		close(recvPipe[1]);
		// Now in an infinite loop, check for data in the recvPipe and send it
		while (1) {
			// Read the data a byte at a time checking for the NULL character
			int n = 0;
			char buf[256];
			while ((buf[n++] = getc(recvPipe[0])) != 0 && n < 255) continue;
			if (n > 1) {
				// We got some data! We now need to send it.
				std::string str(buf, n); // Convert it to a string
				std::string response = send_packet(str);
				// Check response
				if (response != "ACK") {
					//TODO error handling
					break;
				}
			}
			if (n <= 1) {
				// We have finished getting all the data
				std::string response = send_packet("FINISHED");
				// Now we receive data from the server
				while (response != "FINISHED") {
					// Write the response to the main program
					write(sendPipe[1], response, sizeof (response));
					response = send_packet("ACK"); // Acknowledge data received
				}
			}
		}
	} else {
		// Assign the pipes for the main process and close the un-needed ones
		pipes[0] = sendPipe[0];
		pipes[1] = recvPipe[1];
		close(sendPipe[1]);
		close(recvPipe[0]);
		return 0;
	}
	return 0;
}

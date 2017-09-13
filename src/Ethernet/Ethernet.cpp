#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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

void Client::open_connection() {
	int n;
	char buffer[256];
	if (connect(m_sockfd, (struct sockaddr *) &m_serv_addr, sizeof (m_serv_addr)) < 0)
		error("ERROR connecting");
	while (1) {
		printf("Please enter the message: ");
		bzero(buffer, 256);
		fgets(buffer, 255, stdin);
		n = write(m_sockfd, buffer, strlen(buffer));
		if (n < 0)
			error("ERROR writing to socket");

		if (strncmp(buffer, "E", 1) == 0) {
			printf("Disconnecting from server...");
			break;
		}
		bzero(buffer, 256);
		n = read(m_sockfd, buffer, 255);
		if (n < 0)
			error("ERROR reading from socket");
		printf("%s\n", buffer);
	}
	close(m_sockfd);
}

Client::~Client() {
}

std::string Client::send_packet(std::string packet) {
	/*
	 * Sends a data packet to the server. General format is:
	 * [SYNC] [MSGID] [MSGLEN] [DATA] [CRC]
	 * Data uses consistent overhead byte stuffing
	 */

}


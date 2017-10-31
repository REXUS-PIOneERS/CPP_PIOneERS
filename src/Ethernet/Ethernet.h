/**
 * REXUS PIOneERS - Pi_1
 * Ethernet.h
 * Purpose: Class and function definitions for the Server and Client classes
 *
 * @author David Amison
 * @version 2.0 12/10/2017
 *
 */
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <error.h>  // For errno

#include "comms/pipes.h"
#include "logger/logger.h"

#ifndef ETHERNET_H
#define ETHERNET_H

class Server {
	int m_sockfd, m_newsockfd, m_port, m_pid;
	socklen_t m_clilen;
	struct sockaddr_in m_serv_addr, m_cli_addr;
	char m_buf[256];
	Logger log;

public:
	comms::Pipe m_pipes;

	/**
	 * Constructor for the Server class
	 *
	 * @param port: Port for communication with clients
	 */
	Server(const int port) : log("/Docs/Logs/server"), m_port(port) {
		log.start_log();
		setup();
	}

	/**
	 * Starts the server running as a child process ready for accepting
	 * connections from a client.
	 * @return Pipe class for communication charing data with child process.
	 */
	comms::Pipe run(std::string filename);

	~Server();

private:
	/**
	 * Sets up basic variables for creating a server
	 * @return 0 = success, otherwise = failure
	 */
	int setup();

};

class Client {
	int m_port, m_pid;
	std::string m_host_name;
	int m_sockfd;
	struct sockaddr_in m_serv_addr;
	struct hostent *m_server;
	Logger log;
public:
	comms::Pipe m_pipes;

	/**
	 * Constructor for the Client class
	 * @param port: Port for connecting to the server
	 * @param host_name: Name or IP address of the server
	 */
	Client(const int port, const std::string host_name)
	: log("/Docs/Logs/client"), m_port(port), m_host_name(host_name) {
		log.start_log();
	}

	/**
	 * Opens a connection with the server as a separate process
	 * @return Pipe for communication with the process.
	 */
	comms::Pipe run(std::string filename);

	/**
	 * Opens a new connection with the server
	 * @return 0 = success, otherwise = failure
	 */
	int open_connection();

	/**
	 * Closes connection with the server
	 * @return 0 = success, otherwise = failure
	 */
	int close_connection();

	~Client();
private:
	/**
	 * Called by constructor. Sets up basic variables needed for the client
	 * @return 0 = success, otherwise = failure
	 */
	int setup();
};

class EthernetException {
public:

	EthernetException(const std::string error) {
		m_error = error + " :" + std::strerror(errno);
	}

	const char * what() {
		return m_error.c_str();
	}

private:

	std::string m_error;
};

#endif /* ETHERNET_H */


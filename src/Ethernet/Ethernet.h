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
	int _sockfd, _newsockfd, _port, _pid;
	socklen_t m_clilen;
	struct sockaddr_in _serv_addr, _cli_addr;
	Logger Log;

public:
	comms::Pipe m_pipes;

	/**
	 * Constructor for the Server class
	 *
	 * @param port: Port for communication with clients
	 */
	Server(const int port) : _port(port), Log("/Docs/Logs/server") {
		Log.start_log();
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
	int _port, _pid;
	std::string _host_name;
	int _sockfd;
	struct sockaddr_in _serv_addr;
	struct hostent *_server;
	Logger Log;
public:
	comms::Pipe m_pipes;

	/**
	 * Constructor for the Client class
	 * @param port: Port for connecting to the server
	 * @param host_name: Name or IP address of the server
	 */
	Client(const int port, const std::string host_name)
	: _port(port), _host_name(host_name), Log("/Docs/Logs/client") {
		Log.start_log();
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
		_error = error + " :" + std::strerror(errno);
	}

	const char * what() {
		return _error.c_str();
	}

private:

	std::string _error;
};

#endif /* ETHERNET_H */


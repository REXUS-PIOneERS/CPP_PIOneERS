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
	socklen_t _clilen;
	struct sockaddr_in _serv_addr, _cli_addr;
	Logger Log;
	comms::Pipe _pipes;
	bool _connected = false;

public:

	/**
	 * Constructor for the Server class
	 *
	 * @param port: Port for communication with clients
	 */
	Server(const int port) : _port(port), Log("/Docs/Logs/server") {
		Log.start_log();
	}

	/**
	 * Checks the status of pipe and ethernet communications for the server.
	 * @return 0 if all is good
	 * 0bXXXXXXX1 represents read end of pipe is unavailbale
	 * 0bXXXXXX1X represents write end of pipe is unavailable
	 * 0bXXXXX1XX represents ethernet communication is down
	 */
	int status();

	/**
	 * Starts the server running as a child process ready for accepting
	 * connections from a client.
	 * @return Pipe class for communication charing data with child process.
	 */
	comms::Pipe run(std::string filename);


	/**
	 * Sets up basic variables for creating a server
	 * @return 0 = success, otherwise = failure
	 */
	int setup();

	~Server();

};

class Client {
	int _port, _pid;
	std::string _host_name;
	int _sockfd;
	struct sockaddr_in _serv_addr;
	struct hostent *_server;
	Logger Log;
	comms::Pipe _pipes;
	bool _connected = false;
public:

	/**
	 * Constructor for the Client class
	 * @param port: Port for connecting to the server
	 * @param host_name: Name or IP address of the server
	 */
	Client(const int port, const std::string host_name)
	: _port(port), _host_name(host_name), Log("/Docs/Logs/client") {
		Log.start_log();
	}

	bool active() {
		return _connected;
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

	/**
	 * Called by constructor. Sets up basic variables needed for the client
	 * @return 0 = success, otherwise = failure
	 */
	int setup();

	~Client();

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


/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Ethernet.h
 * Author: David
 *
 * Created on 05 September 2017, 19:05
 */
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef ETHERNET_H
#define ETHERNET_H

class Server {
public:
	Server(int port);
	int setup();
	~Server();
private:
	int m_sockfd, m_newsockfd, m_port, m_pid;
	socklen_t m_clilen;
	struct sockaddr_in m_serv_addr, m_cli_addr;
	char m_buf[256];

};

class Client {
public:
	Client(int port, std::string host_name);

	int run(int pipes[2]);
	int open_connection();
	std::string send_packet(std::string packet);
	int close_connection();
	~Client();
private:
	int m_port, m_pid;
	std::string m_host_name;
	int m_sockfd;
	struct sockaddr_in m_serv_addr;
	struct hostent *m_server;

	int setup();
	int open_connection();
};

#endif /* ETHERNET_H */


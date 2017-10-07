/*
 * Class to control the creation of, reading to and writing from pipes
 * in c++.
 */

/*
 * File:   pipes.h
 * Author: David
 *
 * Created on 04 October 2017, 14:51
 */

#ifndef PIPES_H
#define PIPES_H

#include <string>
#include <cstring>
#include <stdint.h>
#include <error.h>  // For errno

class Pipe {
public:
	Pipe();
	// Access Functions
	int getReadfd();
	int getWritefd();
	// Wrap of the fork process
	int Fork();
	// Read and write functions
	int strwrite(std::string);
	int binwrite(void* data, int n);
	std::string strread();
	int binread(void* data, int n = 0);

	void close_pipes();
	~Pipe();

private:
	int m_pipes1[2];
	int m_pipes2[2];
	int m_par_read;
	int m_par_write;
	int m_ch_read;
	int m_ch_write;
	int m_pid; // 0 => child, >0 => parent
};

class PipeException {
public:

	PipeException(std::string error) {
		m_error = error + std::strerror(errno);
	}

	const char * what() {
		return m_error.c_str();
	}

private:
	std::string m_error;
};


#endif /* PIPES_H */


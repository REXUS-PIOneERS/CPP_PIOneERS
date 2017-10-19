/**
 * REXUS PIOneERS - Pi_1
 * pipes.h
 * Purpose: Class definition for implementation of pipes class for data
 *			communication between processes
 *
 * @author David Amison
 * @version 1.0 10/10/2017
 */

#ifndef PIPES_H
#define PIPES_H

#include <string>
#include <cstring>
#include <stdint.h>
#include <error.h>  // For errno

class Pipe {
public:
	/**
	 * Default constructor for the Pipe class. Created all pipes ready for
	 * communication.
	 */
	Pipe();

	/**
	 * Get the read file descriptor
	 * @return Read file descriptor based on the process we are in. Returns -1 if
	 * processes are yet to be forked.
	 */
	int getReadfd();

	/**
	 * Get the write file descriptor
	 * @return Write file descriptor based on the process we are in. Returns -1 if
	 * processes are yet to be forked.
	 */
	int getWritefd();

	/**
	 * Class specific implementation of the fork() system call. Splits the processes
	 * and deals with closing unneeded pipes in both the parent and child process.
	 *
	 * @return pid of the forked process as with fork(), 0 if we are in the child
	 *		   process, >0 otherwise
	 */
	int Fork();

	/**
	 * Writes a string to the pipe
	 *
	 * @param str: String to write
	 * @return Number of bytes written
	 */
	int strwrite(const std::string);

	/**
	 * Writes binary data to the pipe.
	 *
	 * @param data: Buffer array containing the data
	 * @param n: Number of bytes to write (i.e. size of data array)
	 * @return The number of bytes written
	 */
	int binwrite(const void* data, int n);

	/**
	 * Read a string from the pipe
	 *
	 * @return String read from the pipe (empty std::string if nothing to read)
	 */
	std::string strread();

	/**
	 * Read binary data from the pipe.
	 *
	 * @param data: Buffer to store the data
	 * @param n: Maximum number of bytes to read (i.e. size of data buffer)
	 * @return Number of bytes read (0 if pipe has no data for reading)
	 */
	int binread(void* data, int n);

	/**
	 * Closes all the file descriptors
	 */
	void close_pipes();

	/**
	 * Does noting
	 */
	~Pipe();

private:
	int m_pipes1[2];
	int m_pipes2[2];
	int m_par_read;
	int m_par_write;
	int m_ch_read;
	int m_ch_write;
	int m_pid = -1; // 0 => child, >0 => parent
};

class PipeException {
public:

	PipeException(std::string error) {
		m_error = error + ": " + std::strerror(errno);
	}

	const char * what() {
		return m_error.c_str();
	}

private:
	std::string m_error;
};


#endif /* PIPES_H */


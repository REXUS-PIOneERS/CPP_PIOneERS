/*
 * Function implementations for the Pipe class
 */
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "pipes.h"

bool poll_read(int fd) {
	// Checks a file descriptor is ready for reading
	struct pollfd fds[1];
	int timeout = 0;
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	// Check if the file descriptor is ready for reading
	if (poll(fds, 1, timeout))
		return fds[0].revents & POLLIN;
}

Pipe::Pipe() {
	if (pipe(m_pipes1))
		throw PipeException("ERROR: Unable to create pipes");
	if (pipe(m_pipes2))
		throw PipeException("ERROR: Unable to create pipes");
	m_ch_read = m_pipes1[0];
	m_par_write = m_pipes1[1];
	m_par_read = m_pipes2[0];
	m_ch_write = m_pipes2[1];
}

int Pipe::Fork() {
	// Forks the process and closes pipes that are not for use by that process
	if ((m_pid = fork()) == 0) {
		// This is the child process
		close(m_par_read);
		close(m_par_write);
		return m_pid;
	} else {
		close(m_ch_read);
		close(m_ch_write);
		return m_pid;
	}
}

int Pipe::getReadfd() {
	if (m_pid)
		return m_par_read;
	else
		return m_ch_read;
}

int Pipe::getWritefd() {
	if (m_pid)
		return m_par_write;
	else
		return m_ch_write;
}

int Pipe::binwrite(char* data, int n) {
	// Write n bytes of data to the pipe.
	int write_fd = (m_pid) ? m_par_write : m_ch_write;
	if (n == write(write_fd, data, n))
		return n;
	else
		throw PipeException("ERROR: Failed to write data to pipe");
}

int Pipe::strwrite(std::string str) {
	// Write a string of characters to the pipe
	int write_fd = (m_pid) ? m_par_write : m_ch_write;
	int n = str.length();
	if (n == write(write_fd, str.c_str(), n))
		return n;
	else
		throw PipeException("ERROR: Failed to write string to pipe");
}

int Pipe::binread(char* data, int n) {
	// Reads upto n bytes into the character array, returns bytes read
	int read_fd = (m_pid) ? m_par_read : m_ch_read;
	// Check if there is any data to read
	if (!poll_read(read_fd))
		return 0;
	int bytes_read = read(read_fd, data, n);
	return bytes_read;
}

std::string Pipe::strread() {
	int read_fd = (m_pid) ? m_par_read : m_ch_read;
	// Check if there is anything to read
	if (!poll_read(read_fd))
		return std::string();
	char buf[256];
	int n = read(read_fd, buf, 255);
	if (n < 0)
		return std::string();
	else {
		buf[n] = '\0';
		std::string rtn(buf);
		return rtn;
	}
}


Pipe::~Pipe() {
	if (m_pid == 0) {
		close(m_ch_read);
		close(m_ch_write);
	}
	else {
		close(m_par_read);
		close(m_par_write);
	}
}

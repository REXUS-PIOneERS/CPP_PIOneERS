/**
 * REXUS PIOneERS - Pi_1
 * pipes.cpp
 * Purpose: Function declarations for implementation of pipes class for data
 *			communication between processes
 *
 * @author David Amison
 * @version 1.0 10/10/2017
 */
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#include "pipes.h"

/**
 * Check whether a file descriptor is ready for reading
 *
 * @param fd, file descriptor to be checked
 * @return boolean indicating whether fd is ready for read
 */
bool poll_read(const int fd) {
	// Checks a file descriptor is ready for reading
	struct pollfd fds[1];
	int timeout = 0;
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	// Check if the file descriptor is ready for reading
	if (poll(fds, 1, timeout))
		return fds[0].revents & POLLIN;
}

/**
 * Check whether a file descriptor is ready for writing
 *
 * @param fd: File descriptor to be checked
 * @return Boolean indicating whether fd is ready for write
 */
int poll_write(const int fd) {
	// Checks if a file descriptor is available for reading to
	struct pollfd fds[1];
	int timeout = 0;
	fds[0].fd = fd;
	fds[0].events = POLLOUT | POLLHUP;
	if (poll(fds, 1, timeout)) {
		if (fds[0].revents & POLLHUP)
			return -2;
		else if (fds[0].revents & POLLOUT)
			return 0;
		else
			return -1;
	}
}

Pipe::Pipe() {
	// We'll handle pipe problems ourself!
	signal(SIGPIPE, SIG_IGN);
	if (pipe(m_pipes1))
		throw PipeException("ERROR making pipes");
	if (pipe(m_pipes2))
		throw PipeException("ERROR making pipes");
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
	if (m_pid < 0)
		return -1;
	else if (m_pid > 0)
		return m_par_read;
	else
		return m_ch_read;
}

int Pipe::getWritefd() {
	if (m_pid < 0)
		return -1;
	else if (m_pid > 0)
		return m_par_write;
	else
		return m_ch_write;
}

int Pipe::binwrite(const void* data, int n) {
	// Write n bytes of data to the pipe.
	//int write_fd = (m_pid) ? m_par_write : m_ch_write;
	int fd = getWritefd();
	if (n == write(fd, data, n))
		return n;
	else if (n == -1)
		throw PipeException("ERROR writing to pipe");
	else
		return 0;
}

int Pipe::strwrite(const std::string str) {
	// Write a string of characters to the pipe
	//int write_fd = (m_pid) ? m_par_write : m_ch_write;
	int fd = getWritefd();
	int n = str.length();
	if (n == write(fd, str.c_str(), n))
		return n;
	else
		throw PipeException("ERROR writing to pipe");
}

int Pipe::binread(void* data, int n) {
	// Reads upto n bytes into the character array, returns bytes read
	int fd = (m_pid) ? m_par_read : m_ch_read;
	// Check if there is any data to read
	if (!poll_read(fd))
		return 0;
	int bytes_read = read(fd, data, n);
	if (bytes_read == -1)
		throw PipeException("ERROR: Pipe is broken");
	else
		return bytes_read;
}

std::string Pipe::strread() {
	int fd = getReadfd();
	// Check if there is anything to read
	if (!poll_read(fd))
		return std::string();
	char buf[256];
	int n = read(fd, buf, 255);
	if (n == -1)
		throw PipeException("ERROR: Pipe is broken");
	else {
		buf[n] = '\0';
		std::string rtn(buf);
		return rtn;
	}
}

void Pipe::close_pipes() {
	if (m_pid < 0) {
		close(m_ch_read);
		close(m_ch_write);
		close(m_par_read);
		close(m_par_write);
	} else if (m_pid == 0) {
		close(m_ch_read);
		close(m_ch_write);
	} else {
		close(m_par_read);
		close(m_par_write);
	}
}

Pipe::~Pipe() {
}

/*
 * Function implementations for the Pipe class
 */
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#include "pipes.h"

bool poll_read(int fd) {
	// Checks a file descriptor is ready for reading
	signal(SIGPIPE, SIG_IGN);
	struct pollfd fds[1];
	int timeout = 0;
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	// Check if the file descriptor is ready for reading
	if (poll(fds, 1, timeout))
		return fds[0].revents & POLLIN;
}

int poll_write(int fd) {
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

int Pipe::binwrite(void* data, int n) {
	// Write n bytes of data to the pipe.
	int write_fd = (m_pid) ? m_par_write : m_ch_write;
	int poll_val = poll_write(write_fd);
	if (poll_val == 0) {
		if (n == write(write_fd, data, n))
			return n;
		else
			throw PipeException("ERROR: Pipe is broken");
	} else
		throw PipeException("ERROR: Pipe unavailbale for writing");

}

int Pipe::strwrite(std::string str) {
	// Write a string of characters to the pipe
	int write_fd = (m_pid) ? m_par_write : m_ch_write;
	int poll_val = poll_write(write_fd);
	if (poll_val == 0) {
		int n = str.length();
		if (n == write(write_fd, str.c_str(), n))
			return n;
		else if (n == -1)
			throw PipeException("ERROR: Pipe is broken");
		else
			throw PipeException("ERROR: Pipe unavailable for writing");
	} else
		throw PipeException("ERROR: Pipe unavailable for writing");
}

int Pipe::binread(void* data, int n) {
	// Reads upto n bytes into the character array, returns bytes read
	int read_fd = (m_pid) ? m_par_read : m_ch_read;
	// Check if there is any data to read
	if (!poll_read(read_fd))
		return 0;
	int bytes_read = read(read_fd, data, n);
	if (bytes_read == -1)
		throw PipeException("ERROR: Pipe is broken");
	else
		return bytes_read;
}

std::string Pipe::strread() {
	int read_fd = (m_pid) ? m_par_read : m_ch_read;
	// Check if there is anything to read
	if (!poll_read(read_fd))
		return std::string();
	char buf[256];
	int n = read(read_fd, buf, 255);
	if (n == -1)
		throw PipeException("ERROR: Pipe is broken");
	else {
		buf[n] = '\0';
		std::string rtn(buf);
		return rtn;
	}
}

void Pipe::close_pipes() {
	if (m_pid == 0) {
		close(m_ch_read);
		close(m_ch_write);
	} else {
		close(m_par_read);
		close(m_par_write);
	}
}

Pipe::~Pipe() {
	close_pipes();
}

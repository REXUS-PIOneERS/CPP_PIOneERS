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

namespace comms {

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
	bool poll_write(const int fd) {
		// Checks if a file descriptor is available for reading to
		struct pollfd fds[1];
		int timeout = 0;
		fds[0].fd = fd;
		fds[0].events = POLLOUT | POLLHUP;
		if (poll(fds, 1, timeout)) {
			if (fds[0].revents & POLLHUP)
				return false;
			else if (fds[0].revents & POLLOUT)
				return true;
			else
				return false;
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
		return (m_pid > 0) ? m_par_read : m_ch_read;
	}

	int Pipe::getWritefd() {
		if (m_pid < 0)
			return -1;
		return (m_pid > 0) ? m_par_write : m_ch_write;
	}

	int Pipe::binwrite(const void* data, int n) {
		// Write n bytes of data to the pipe.
		int fd = getWritefd();
		if (fd < 0)
			return -1;
		if (!poll_write(fd))
			return -2;
		if (n == write(fd, data, n))
			return n;
		else
			return -3;
	}

	int Pipe::binread(void* data, int n) {
		// Reads upto n bytes into the character array, returns number of bytes read
		int fd = getReadfd();
		if (fd < 0)
			return -1;
		if (!poll_read(fd))
			return -2;
		int bytes_read = read(fd, data, n);
		if (bytes_read == -1)
			return -3;
		else
			return bytes_read;
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

}
#ifndef _RADIOCOM
#define _RADIOCOM

#include "protocol.h"
#include <queue>
#include <iostream>
#include <iterator>
#include <algorithm>



namespace comms {

	class Transceiver {
	public:

		Transceiver(int fd) {
			_fd_send = fd;
			_fd_recv = fd;
		}

		~Transceiver() = default;

		/**
		   Try to get a packet from the file descriptor.
		   @params
		   p: Packet to be received
		   @return
		   0: Success.
		   -1: No packets in queue
		 */
		int recvPacket(Packet *p);

		/**
		   Push a packet to the send queue
		   @params
		   p: Packet to be sent
		 */
		int sendPacket(Packet *p);

		int recvBytes(void* data, int n);

		int sendBytes(void* data, int n);

	protected:
		int _fd_send;
		int _fd_recv;

	private:
	};
}
#endif

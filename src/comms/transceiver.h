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

		int sendBytes(const void* data, int n);

	protected:
		int _fd_send;
		int _fd_recv;

	private:
		PacketChecker _checker;
	};

	class PacketChecker {
		uint8_t _buf[24];
		bool _flag = false;
		int _index = 0;

	public:

		PacketChecker() {
		}

		/**
		 * Puches the next byte into the checker
		 * @param byte; the byte to be pushed
		 * @return 0 when no packet to return, else the length of the packet
		 */
		int push_byte(uint8_t byte);

		/**
		 * Gets the packet from the buffer
		 * @param p will hold the packet
		 * @return false of no packet available yet
		 */
		bool get_packet(comms::Packet *p);
	};
}
#endif

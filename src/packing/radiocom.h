#ifndef _RADIOCOM
#define _RADIOCOM

#include "protocol.h"
#include <queue>
#include <iostream>
#include <iterator>
#include <algorithm>
#include "pipes/pipes.h"


namespace rfcom {

	/*Radio communication module base class*/
	class ComModule {
	protected:
		byte2_t _crc_gen; //CRC16 generator polynomial
		std::queue<Packet*> _pdu_send_queue; //A queue of protocal data units
		std::queue<Packet*> _pdu_recv_queue;

	public:

		ComModule(byte2_t gen) {
			_crc_gen = gen;
		}

		virtual ~ComModule() {
			while (!_pdu_send_queue.empty()) {
				delete _pdu_send_queue.front();
				_pdu_send_queue.pop();
			}
			while (!_pdu_recv_queue.empty()) {
				delete _pdu_recv_queue.front();
				_pdu_recv_queue.pop();
			}
		}

		void setCRCGen(byte2_t gen) {
			_crc_gen = gen;
		}

		byte2_t getCRCGen() const {
			return _crc_gen;
		}
	};

	class Transceiver : public ComModule {
	public:

		Transceiver(byte2_t gen = CRC16_GEN_BUYPASS, int fd) {
			_fd_send = fd;
			_fd_recv = fd;
			_crc_gen = gen;
		}

		Transeiver(byte2_t gen = CRC16_GEN_BUYPASS, Pipe pipes) : ComModule(gen) {
			_fd_send = pipes.getWritefd();
			_fd_recv = pipes.getReadfd();
		}

		~Transceiver() = default;

		void setCRCGen(byte2_t gen) {
			_crc_gen = gen;
		}

		byte2_t getCRCGen() {
			return _crc_gen;
		}

		/**
		   Try to pop the next packet from receive queue and unpack it.
		   @params
		   id: the reference of ID.
		   index: the reference packet index.
		   p_data: starting position of data buffer.
		   len: the actual length of buffer. Depends on ID
		   @return
		   0: Success.
		   -1: COBS decode failure.
		   -2: CRC mismatch.
		 */
		int unpack(Packet *p, byte1_t& id, byte2_t& index, byte1_t* p_data);

		/**
		   Try to pop the next packet from the receive queue.
		   @params
		   p: Packet to be received
		   @return
		   0: Success.
		   -1: No packets in queue
		 */
		int recvPacket(Packet *p);

		/**
		   Try to pack up and push the packet into the send queue.
		   @params
		   id
		   index
		   p_data:starting position of data buffer.
		   @return
		   0: Success.
		   -1: invalid id.
		 */
		int pack(Packet *p, byte1_t id, byte2_t index, byte1_t* p_pata);

		/**
		   Push a packet to the send queue
		   @params
		   p: Packet to be sent
		 */
		void sendPacket(Packet *p);

	protected:
		byte2_t _crc_gen;
		int _fd_send;
		int _fd_recv;

	private:
	};
}
#endif

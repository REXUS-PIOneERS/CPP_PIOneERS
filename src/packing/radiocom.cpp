#include <cstring>
#include "radiocom.h"
#include <unistd.h>

namespace rfcom {

	int Transceiver::unpack(Packet &p, byte1_t& id, byte2_t& index, byte1_t* p_data) {
		//COBS decode failure
		if (!Protocol::cobsDecode(&(p.ohb), 23, p.sync))
			return -1;

		//CRC mismatch
		if (Protocol::crc16Gen(&(p.ID), 19, CRC16_GEN_BUYPASS) != p.checksum)
			return -2;

		id = p.ID;
		index = p.index;
		memcpy(p_data, p.data, sizeof (p.data));

		return 0;
	}

	int Transceiver::recvPacket(Packet *p) {
		int n = read(_fd_recv, (void*)p, 30);
		return n;
	}

	int Transceiver::pack(Packet &p, byte1_t id, byte2_t index, byte1_t* p_data) {
		size_t actual_len;
		//id invalid
		if (!(actual_len = lengthByID(id)))
			return -1;
		p.sync = 0x00;
		p.ID = id;
		p.index = index;

		memcpy(p.data, p_data, actual_len);
		memset(p.data + actual_len, '\0', sizeof (p.data) - actual_len);

		//CRC
		p.checksum = Protocol::crc16Gen(&(p.ID), 19, CRC16_GEN_BUYPASS);
		//COBS
		Protocol::cobsEncode(&(p.ohb), 23, p.sync);


		return 0;
	}

	int Transceiver::sendPacket(Packet *p) {
		int n = write(_fd_send, (void*)p, 30);
		return n;
	}
}

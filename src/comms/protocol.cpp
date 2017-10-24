#include "protocol.h"
#include <iostream>

namespace comms {

	byte2_t Protocol::crc16Gen(const byte1_t* pos, size_t len, byte2_t generator) {
		if (pos == NULL)
			return 0;

		byte2_t checksum = 0x0000;
		int bit_count;

		while (len--) {
			checksum ^= *pos++;
			for (bit_count = 0; bit_count < 8; ++bit_count)
				checksum = checksum & 0x8000 ? (checksum << 1) ^ generator : checksum << 1;
		}

		for (bit_count = 0; bit_count < 8; ++bit_count)
			checksum = checksum & 0x8000 ? (checksum << 1) ^ generator : checksum << 1;

		return checksum;
	}

	byte1_t* Protocol::cobsEncode(byte1_t* pos, size_t len, byte1_t delim) {
		if (pos == NULL || len < 2)
			return pos;

		*(pos + len - 1) = 0x00;
		size_t jump = 0;

		for (size_t offset = --len; offset > 0; --offset, ++jump)
			if (pos[offset] == delim) {
				pos[offset] = jump;
				jump = 0;
			}

		*pos = jump;
		return pos;
	}

	bool Protocol::cobsDecode(byte1_t* pos, size_t len, byte1_t delim) {
		if (pos == NULL || len < 2)
			return false;

		byte1_t* next_pos = pos + *pos;
		size_t offset = 0;
		byte1_t* off_end_pos = pos + len;
		while (*next_pos != 0 && next_pos < off_end_pos) {
			offset = *next_pos;
			*next_pos = delim;
			next_pos += offset;
		}

		if (next_pos != off_end_pos - 1)
			return false;

		return true;
	}

	int pack(Packet &p, byte1_t id, byte2_t index, void* p_data) {
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
		p.checksum = Protocol::crc16Gen(&(p.ID), 19, crc_poly);
		//COBS
		Protocol::cobsEncode(&(p.ohb), 23, p.sync);


		return 0;
	}

	int unpack(Packet &p, byte1_t& id, byte2_t& index, void* p_data) {
		//COBS decode failure
		if (!Protocol::cobsDecode(&(p.ohb), 23, p.sync))
			return -1;

		//CRC mismatch
		if (Protocol::crc16Gen(&(p.ID), 19, crc_poly) != p.checksum)
			return -2;

		id = p.ID;
		index = p.index;
		memcpy(p_data, p.data, sizeof (p.data));

		return 0;
	}
}

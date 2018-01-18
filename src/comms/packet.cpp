#include <cstdint>
#include <iostream>
#include "packet.h"

namespace comms {

	std::ostream& operator<<(std::ostream &o, Packet &p) {
		o << "ID: " << (int) p.ID << " Index: " << p.index << " Data: ";
		if (p.ID == ID_MSG1) {
			for (int i = 0; i < 16; i++)
				o << (char) p.data[i];
		} else {
			for (int i = 0; i < 16; i++)
				o << (int) p.data[i] << ',';
		}
		o << p.checksum;

		return o;
	}

	/**
	   Determine the length of actual data in a packet with id
	   @params
	   id: packet ID
	   @return
	   Length of actual data in bytes. Return 0 if id invalid.
	 */
	size_t lengthByID(byte1_t id) {
		//Status or message
		switch (id) {
			case ID_MSG1:
			case ID_MSG2:
			case ID_STATUS1:
			case ID_STATUS2:
				return 16;
			case ID_DATA1:
			case ID_DATA3:
			case ID_DATA4:
				return 12;
			case ID_DATA2:
				return 10;
			default:
				return 0;
		}
	}
}

#include <cstdint>
#include <iostream>
#include "packet.h"

namespace comms {

	std::ostream& operator<<(std::ostream &o, Packet &p) {
		o << "ID: " << (int) p.ID << " Index: " << p.index << " Data: ";
		if (p.ID == 0xc0) {
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
		if (id & 0b11000000)
			return 16;
			//Measured data
		else {
			switch (id & 0b00111111) {
					//acc/gyr from either IMUs
				case 0b00010000:
				case 0b00100000:
					return 12;
					//mag/time from IMU_1
				case 0b00010001:
					return 10;
					//mag/imp/time from IMU_2
				case 0x00100001:
					return 12;
					//Invalid id
				default:
					return 0;
			}
		}
	}
}

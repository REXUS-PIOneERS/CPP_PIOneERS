#ifndef _PACKET
#define _PACKET

#include <cstdint>
#include <iostream>

#define crc_poly 0x724E  // Polynomail for calculating checksim

#define ID_MSG1  0b10010000 // Message from Pi 1
#define ID_MSG2  0b10100000 // Message from Pi 2
#define ID_STATUS1 0b01010000 // Status from Pi 1
#define ID_STATUS2 0b01100000 // Status from Pi 2
#define ID_DATA1 0b00010000 // Acc/Gyr from Pi 1
#define ID_DATA2 0b00010001 // Mag/Time from Pi 1
#define ID_DATA3 0b00100000 // Acc/Gyr from Pi 2
#define ID_DATA4 0b00100010 //Mag/Time from Pi 2

namespace comms {
	typedef uint8_t byte1_t;
	typedef uint16_t byte2_t;

	/*
	 * Make sure this structure is tightly arranged in memory.
	 * Structure is:
	 * byte 1: Sync Byte (0)
	 * byte 2: Overhead Byte (for COBS)
	 * byte 3: ID
	 * byte 4-5: Index of packet
	 * byte 6-21: Data
	 * byte 22-23: Checksum
	 * byte 24: End of Packet (0)
	 */
#pragma pack(push, 1)

	struct Packet {
		byte1_t sync;
		byte1_t ohb;
		byte1_t ID;
		byte2_t index;
		byte1_t data[16];
		byte2_t checksum;
		byte1_t eop;
	};

#pragma pack(pop)

	std::ostream& operator<<(std::ostream &o, Packet &p);

	/**
	   Determine the length of actual data in a packet with id
	   @params
	   id: packet ID
	   @return
	   Length of actual data in bytes. Return 0 if id invalid.
	 */
	size_t lengthByID(byte1_t id);
}

#endif

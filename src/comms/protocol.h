#ifndef _PROTOCOL
#define _PROTOCOL

#include <cstdio>
#include "packet.h"

#define CRC16_GEN_BUYPASS (0x8005)
#define CRC16_GEN_XMODEM (0x1021)
#define CRC16_GEN_DECTX (0x0589)
#define CRC16_GEN_T10DIFF (0x8BB7)

namespace comms {

	//_Tc --- data type of the CRC generator.
	//CRC generator must not contain leading zeros.

	class Protocol {
	public:
		/**
		  Generate CRC16 check sequence. Checksum initial value 0x0000. Non-reflected output and input.
		  @params
		  pos: starting position of buffer.
		  len: total length of buffer. Length should be at least 1 bytes.
		  generator: CRC16 generator. Ignore the coefficient 1 of power 16.
		  @return
		  CRC checksum.
		 */
		static byte2_t crc16Gen(const byte1_t* pos, size_t len, byte2_t generator);

		/**
		  COBS encode. Unencoded data should be placed in range [p+1, p+len-1). *(p+len-1) should be 0.
		  @params
		  pos: Starting position of buffer being encoded. This buffer should start from ohb, end at eop.
		  len: total length of buffer, in bytes. Length should be at least 2 bytes.
		  delim: delimiter. The character to be replaced.
		  @return
		  pos
		 */
		static byte1_t* cobsEncode(byte1_t* pos, size_t len, byte1_t delim);

		/**
		   COBS decode. *(p+len-1) should be 0.
		   @params
		   pos: Starting position of buffer being decoded. This buffer should start from ohb, end at eop.
		   len: total length of buffer, in bytes. Length should be at least 2 bytes.
		   delim: delimiter. The character to be repalced.
		   @return
		   boolean, indicates the buffer can be correctly decoded.
		 */
		static bool cobsDecode(byte1_t* pos, size_t len, byte1_t delim);

		/**
			Try to pack up the data.
			@params
			p: address of packet for data to be packed into.
			id: id of the packet
			index: index of the packet
			p_data: starting position of data buffer.
			@return
			0: Success.
			-1: invalid id.
		 */
		static int pack(Packet &p, byte1_t id, byte2_t index, byte1_t* p_data);

		/**
		   Try to unpack a packet.
		   @params
		   p: packet to be unpacked
		   id: the reference of ID.
		   index: the reference packet index.
		   p_data: starting position of data buffer.
		   len: the actual length of buffer. Depends on ID
		   @return
		   0: Success.
		   -1: COBS decode failure.
		   -2: CRC mismatch.
		 */
		static int unpack(Packet &p, byte1_t& id, byte2_t& index, byte1_t* p_data);
	};
}

#endif

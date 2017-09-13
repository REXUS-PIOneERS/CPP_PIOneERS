#include <stdint.h>

void CalculateTable_CRC16(uint16_t *table, uint16_t poly) {
	/*
	 * Iterates over all possible byte values and calculates corresponding
	 * CRC results for use in CRC calculations.
	 */

	for (int num = 0; num < 256; num++) {
		uuint16_t curByte = (uint16_t(num << 8);
				// Iterate over each bit to find the CRC value
		for (int16_t bit = 0; bit < 8; bit++) {
			if ((curByte & 0x8000) != 0) {
				curByte <<= 1;
						curByte ^= poly;
			}

			else curByte <<= 1;
			}
		// Add the result to the table
		table[num] = curByte;
	}
}


uint16_t crc16(uint8_t* data, int len, uint16_t poly, uint16_t *table) {
	/*
	 * Data is the data for which the CRC is to be calculated, len is the number
	 * of bytes in the data, poly is the CRC Polynomial to be used, table is a
	 * pre-calculated table of CRC values for bytes 0 through 255.
	 */
	uint16_t crc = 0;
	for (i = 0; i < len; i++) {
		uint8_t pos = (uint8_t) ((crc >> 8) ^ data[i]);
				crc = uint16_t((crc << 8) ^ uint16_t(table[pos]))
	}
	return crc;
}

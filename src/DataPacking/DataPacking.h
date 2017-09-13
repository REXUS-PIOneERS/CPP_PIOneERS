#include<stdint.h>

#ifndef DATAPACKING_H
#define DATAPACKING_H

// Functions for calculating CRC values
void CalculateTable_CRC16(uint16_t *table, uint16_t poly);
uint16_t crc16(uint8_t* data, int len, uint16_t poly, uint16_t *table);

std::string pack_IMU(uint16_t data[10]);
std::string pack_ImP(uint16_t data[11]);
std::string pack_MSG(uint8_t* msg, int len);

void unpack_IMU(uint16_t data[10], char* packet, int len);
void unpack_ImP(uint16_t data[11], char* packet, int len);

#endif /* DATAPACKING_H */


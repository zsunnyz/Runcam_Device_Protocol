#include "stdint.h"
#pragma once

// Functions to calculate crc values of a buffer for error checking
uint8_t calcCrc_dvb_s2(uint8_t *buf, uint8_t numBytes);
uint8_t crc8_calc(uint8_t crc, unsigned char a, uint8_t poly);

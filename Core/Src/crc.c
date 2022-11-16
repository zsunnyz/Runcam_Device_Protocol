#include "crc.h"

/**
 * @brief Calculates the crc dvb s2 value of an array of unsigned integers
 *
 * @param buf (uint8_t*): the array of integers
 * @param numBytes (uint8_t): the  number of elements in the array
 * @return (uint8_t): the crc value of the array
 */
uint8_t calcCrc_dvb_s2( uint8_t *buf, uint8_t numBytes )
{
    uint8_t crc = 0;
    for( uint8_t i=0; i<numBytes; i++ )
        crc = crc8_calc( crc, *(buf+i), 0xd5 );

    return crc;

}

/**
 * @brief helper function for calcCrc_dvb_s2, calculates and appends to the crc
 * 		of a certain element in a buffer
 *
 * @param crc (uint8_t): the previous crc value
 * @param a (unsigned char): the current character to calculate
 * @param poly (uint8_t): the polynomial to be used for calculations
 * @return (uint8_t): the intermediate crc value
 */
uint8_t crc8_calc( uint8_t crc, unsigned char a, uint8_t poly )
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii)
    {
        if (crc & 0x80)
            crc = (crc << 1) ^ poly;
        else
            crc = crc << 1;

    }
    return crc;

}

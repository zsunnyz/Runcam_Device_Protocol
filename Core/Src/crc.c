#include "crc.h"

uint8_t calcCrc_dvb_s2( uint8_t *buf, uint8_t numBytes )
{
    uint8_t crc = 0;
    for( uint8_t i=0; i<numBytes; i++ )
        crc = crc8_calc( crc, *(buf+i), 0xd5 );

    return crc;

}

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

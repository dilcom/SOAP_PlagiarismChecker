#ifndef HASHFUNCTIONS_H
#define HASHFUNCTIONS_H

#include <stdint.h>
#include <stddef.h>

namespace DePlagiarism {    
    //! Hash function, used in redis wrapper
    uint16_t crc16(const char *buf, int len);
    //! CRC32 used in shingles.h
    uint_least32_t Crc32(const unsigned char * buf, size_t len);
}
#endif // HASHFUNCTIONS_H

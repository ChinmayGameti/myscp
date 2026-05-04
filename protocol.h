#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#ifdef __APPLE__
    #include <libkern/OSByteOrder.h>
    #define htobe64(x) OSSwapHostToBigInt64(x)
    #define be64toh(x) OSSwapBigToHostInt64(x)
    // Only define if the system hasn't already
    #ifndef htonl
        #define htonl(x) OSSwapHostToBigInt32(x)
    #endif
    #ifndef ntohl
        #define ntohl(x) OSSwapBigToHostInt32(x)
    #endif
#else
    #include <endian.h>
    #include <arpa/inet.h>
    #include <byteswap.h>
#endif

#define PORT 9000
#define CHUNK_SIZE 65536
#define MAGIC_NUMBER 0x53435058 

struct FileHeader {
    uint32_t magic;           
    uint64_t file_size;       
    char filename[256];       
    uint8_t checksum[32];     
} __attribute__((packed));

#endif

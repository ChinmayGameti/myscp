#ifndef COMMON_H
#define COMMON_H

// 1. Standard C++ Headers
#include <iostream>
#include <string>

// 2. Standard C/POSIX Headers
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

// 3. OpenSSL
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>

// 4. Cross-Platform Endianness
#ifdef __APPLE__
    #include <libkern/OSByteOrder.h>
    #define htobe64(x) OSSwapHostToBigInt64(x)
    #define be64toh(x) OSSwapBigToHostInt64(x)
    #ifndef htonl
        #define htonl(x) OSSwapHostToBigInt32(x)
    #endif
    #ifndef ntohl
        #define ntohl(x) OSSwapBigToHostInt32(x)
    #endif
#else
    #include <endian.h>
    #include <arpa/inet.h>
#endif

// 5. Protocol Definitions
#define CHUNK_SIZE 65536
#define MAGIC_NUMBER 0x53435058 

struct FileHeader {
    uint32_t magic;           
    uint64_t file_size;       
    char filename[256];       
    uint8_t checksum[32];     
} __attribute__((packed));

#endif

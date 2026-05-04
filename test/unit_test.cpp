#include "../common.h"
#include <cassert>
#include <iostream>

void test_endianness_conversion() {
    std::cout << "Running test_endianness_conversion()... ";
    uint32_t magic = MAGIC_NUMBER;
    uint32_t net_magic = htonl(magic);
    uint32_t host_magic = ntohl(net_magic);
    
    assert(magic == host_magic);
    std::cout << "PASSED." << std::endl;
}

void test_header_packing() {
    std::cout << "Running test_header_packing()... ";
    FileHeader header = {};
    // Ensure the struct doesn't have compiler padding padding ruining the stream
    assert(sizeof(header) == sizeof(uint32_t) + sizeof(uint64_t) + 256);
    std::cout << "PASSED." << std::endl;
}

void test_trailer_packing() {
    std::cout << "Running test_trailer_packing()... ";
    FileTrailer trailer = {};
    assert(sizeof(trailer) == 32); // SHA256 is exactly 32 bytes
    std::cout << "PASSED." << std::endl;
}

int main() {
    std::cout << "--- Starting MySCP Unit Tests ---" << std::endl;
    test_endianness_conversion();
    test_header_packing();
    test_trailer_packing();
    std::cout << "✅ All unit tests passed successfully." << std::endl;
    return 0;
}

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "protocol.h"

void calculate_sha256(const char* path, uint8_t* output) {
    FILE* fp = fopen(path, "rb");
    if (!fp) return;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char buffer[CHUNK_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        SHA256_Update(&sha256, buffer, bytes);
    }
    SHA256_Final(output, &sha256);
    fclose(fp);
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <file> [--corrupt-checksum]" << std::endl;
        return 1;
    }

    // Parse flags
    bool inject_checksum_fault = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--corrupt-checksum") == 0) {
            inject_checksum_fault = true;
        }
    }

    struct stat st;
    if (stat(argv[2], &st) != 0) {
        perror("[ERROR] File stat failed");
        return 1;
    }

    // 1. Calculate Real Checksum
    uint8_t hash[32];
    calculate_sha256(argv[2], hash);

    if (inject_checksum_fault) {
        std::cout << "[DEBUG] Intentional corruption of SHA256 checksum..." << std::endl;
        hash[0] ^= 0xFF; // Flip bits in the first byte
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {}; // Zeroed out
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        std::cerr << "[ERROR] Invalid address" << std::endl;
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[ERROR] Connection failed");
        return 1;
    }

    // 2. Send Header
    FileHeader header = {}; // Zeroed out
    header.magic = htonl(MAGIC_NUMBER);
    header.file_size = htobe64(st.st_size);
    strncpy(header.filename, basename((char*)argv[2]), 256);
    memcpy(header.checksum, hash, 32);

    if (send(sock, &header, sizeof(FileHeader), 0) < 0) {
        perror("[ERROR] Header send failed");
        return 1;
    }

    // 3. Stream File
    FILE* fp = fopen(argv[2], "rb");
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        if (send(sock, buffer, bytes_read, 0) < 0) {
            perror("[ERROR] Data stream failed");
            break;
        }
    }

    std::cout << "[SUCCESS] File sent." << std::endl;
    fclose(fp);
    close(sock);
    return 0;
}

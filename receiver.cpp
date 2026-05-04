#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <string.h>
#include "protocol.h"

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    // SO_REUSEADDR prevents the "Address already in use" error if you restart quickly
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = {}; // Zeroed out
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERROR] Bind failed");
        return 1;
    }
    listen(server_fd, 5);
    std::cout << "[LISTENING] Port " << PORT << "..." << std::endl;

    while(true) {
        int client_sock = accept(server_fd, NULL, NULL);
        if (client_sock < 0) { perror("[ERROR] Accept failed"); continue; }

        FileHeader header;
        if (recv(client_sock, &header, sizeof(FileHeader), 0) <= 0) {
            close(client_sock); continue;
        }

        if (ntohl(header.magic) != MAGIC_NUMBER) {
            std::cerr << "[CRITICAL] Protocol mismatch! Dropping connection." << std::endl;
            close(client_sock); continue;
        }

        uint64_t file_size = be64toh(header.file_size);
        std::cout << "[RECEIVING] " << header.filename << " | Size: " << file_size << " bytes" << std::endl;
        
        FILE* fp = fopen(header.filename, "wb");
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        char buffer[CHUNK_SIZE];
        uint64_t total_received = 0;
        
        while (total_received < file_size) {
            ssize_t bytes = recv(client_sock, buffer, CHUNK_SIZE, 0);
            if (bytes <= 0) break;
            
            fwrite(buffer, 1, bytes, fp);
            SHA256_Update(&sha256, buffer, bytes);
            total_received += bytes;
        }
        fclose(fp);

        // Finalize Hash
        uint8_t final_hash[32];
        SHA256_Final(final_hash, &sha256);

        if (memcmp(final_hash, header.checksum, 32) == 0) {
            std::cout << "✅ [VERIFIED] " << header.filename << " matches checksum." << std::endl;
        } else {
            std::cerr << "❌ [ALERT] Checksum mismatch! File may be corrupted." << std::endl;
        }
        close(client_sock);
    }
    return 0;
}

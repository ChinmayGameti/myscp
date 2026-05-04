#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--server") != 0) {
        std::cerr << "Usage: " << argv[0] << " --server (Invoked automatically via SSH)" << std::endl;
        return 1;
    }

    FileHeader header;
    if (fread(&header, 1, sizeof(FileHeader), stdin) != sizeof(FileHeader)) {
        std::cout << "[CRITICAL] Failed to read header from SSH stream." << std::endl;
        return 1;
    }

    if (ntohl(header.magic) != MAGIC_NUMBER) {
        std::cout << "[CRITICAL] Protocol mismatch! Dropping connection." << std::endl;
        return 1;
    }

    uint64_t file_size = be64toh(header.file_size);
    std::string save_path = header.filename; 

    // Dynamic path parsing
    if (argc >= 3) {
        std::string target_path = argv[2];
        if (target_path.back() == '/') {
            save_path = target_path + header.filename;
        } else {
            save_path = target_path; 
        }
    }

    std::cout << "[REMOTE] Saving to: " << save_path << " (" << file_size << " bytes)" << std::endl;
    
    FILE* fp = fopen(save_path.c_str(), "wb");
    if (!fp) {
        // This exact error will be caught and printed by the sender!
        std::cout << "❌ [ERROR] Cannot open file for writing: " << save_path << ". Check permissions or path existence." << std::endl;
        return 1;
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char buffer[CHUNK_SIZE];
    uint64_t total_received = 0;
    
    while (total_received < file_size) {
        size_t bytes = fread(buffer, 1, CHUNK_SIZE, stdin);
        if (bytes <= 0) break;
        fwrite(buffer, 1, bytes, fp);
        SHA256_Update(&sha256, buffer, bytes);
        total_received += bytes;
    }
    fclose(fp);

    uint8_t final_hash[32];
    SHA256_Final(final_hash, &sha256);

    if (memcmp(final_hash, header.checksum, 32) == 0) {
        std::cout << "✅ [REMOTE VERIFIED] " << header.filename << " matches SHA256 checksum perfectly." << std::endl;
    } else {
        std::cout << "❌ [REMOTE ALERT] Checksum mismatch! File corrupted during transfer." << std::endl;
    }
    
    return 0;
}

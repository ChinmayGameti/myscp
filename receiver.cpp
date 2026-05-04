#include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--server") != 0) {
        std::cerr << "Usage: " << argv[0] << " --server" << std::endl;
        return 1;
    }

    FileHeader header;
    if (fread(&header, 1, sizeof(FileHeader), stdin) != sizeof(FileHeader)) {
        std::cout << "[CRITICAL] Failed to read header." << std::endl;
        return 1;
    }

    if (ntohl(header.magic) != MAGIC_NUMBER) {
        std::cout << "[CRITICAL] Protocol mismatch!" << std::endl;
        return 1;
    }

    header.filename[sizeof(header.filename) - 1] = '\0';

    uint64_t file_size = be64toh(header.file_size);
    std::string save_path = header.filename;

    if (argc >= 3) {
        std::string target_path = argv[2];
        if (target_path.back() == '/') save_path = target_path + header.filename;
        else save_path = target_path; 
    }

    std::cout << "[REMOTE] Saving to: " << save_path << " (" << file_size << " bytes)" << std::endl;
    
    FILE* fp = fopen(save_path.c_str(), "wb");
    if (!fp) {
        std::cout << "❌ [ERROR] Cannot open file: " << save_path << std::endl;
        return 1;
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    char buffer[CHUNK_SIZE];
    uint64_t total_received = 0;
    
    while (total_received < file_size) {
        uint64_t bytes_left = file_size - total_received;
        size_t to_read = (bytes_left < CHUNK_SIZE) ? bytes_left : CHUNK_SIZE;

        size_t bytes = fread(buffer, 1, to_read, stdin);
        if (bytes <= 0) break;

        fwrite(buffer, 1, bytes, fp);
        SHA256_Update(&sha256, buffer, bytes);
        total_received += bytes;
    }
    fclose(fp);

    uint8_t local_hash[32];
    SHA256_Final(local_hash, &sha256);

    FileTrailer trailer;
    if (fread(&trailer, 1, sizeof(FileTrailer), stdin) != sizeof(FileTrailer)) {
        std::cout << "❌ [REMOTE ALERT] Failed to read checksum trailer!" << std::endl;
        return 1;
    }

    if (memcmp(local_hash, trailer.checksum, 32) == 0) {
        std::cout << "✅ [REMOTE VERIFIED] " << header.filename << " matches SHA256 checksum perfectly." << std::endl;
        return 0;
    } else {
        std::cout << "❌ [REMOTE ALERT] Checksum mismatch! File corrupted." << std::endl;
        return 1;
    }
}

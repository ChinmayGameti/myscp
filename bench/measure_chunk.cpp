// bench/measure_chunk.cpp - Throughput vs CHUNK_SIZE micro-benchmark
//
// Reads a file with several chunk sizes, computing SHA-256 on the fly
// (mirroring the sender's hot loop) so we can defend the production
// CHUNK_SIZE value chosen in common.h. Output is a markdown table that
// can be pasted directly into PROJECT_DESIGN.md.

#include <iostream>
#include <chrono>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <sys/stat.h>

#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>

static const size_t CHUNKS[] = {
    4   * 1024,
    16  * 1024,
    64  * 1024,    // production default
    256 * 1024,
    1024 * 1024,
    4    * 1024 * 1024
};

double measure(const char* path, size_t chunk) {
    FILE* fp = fopen(path, "rb");
    if (!fp) { perror("fopen"); return -1.0; }

    std::vector<char> buf(chunk);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    auto start = std::chrono::steady_clock::now();
    size_t bytes;
    uint64_t total = 0;
    while ((bytes = fread(buf.data(), 1, chunk, fp)) > 0) {
        SHA256_Update(&ctx, buf.data(), bytes);
        total += bytes;
    }
    uint8_t digest[32];
    SHA256_Final(digest, &ctx);
    auto end = std::chrono::steady_clock::now();
    fclose(fp);

    double secs = std::chrono::duration<double>(end - start).count();
    if (secs <= 0) return -1.0;
    return (total / 1024.0 / 1024.0) / secs;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }
    const char* path = argv[1];
    struct stat st;
    if (stat(path, &st) != 0) { perror("stat"); return 1; }

    std::cout << "# Chunk-Size Benchmark" << std::endl;
    std::cout << "File: " << path << " (" << st.st_size << " bytes)" << std::endl;
    std::cout << std::endl;
    std::cout << "| Chunk Size | Throughput (MB/s) |" << std::endl;
    std::cout << "| ---: | ---: |" << std::endl;

    for (size_t chunk : CHUNKS) {
        double mbps = measure(path, chunk);
        std::cout << "| " << (chunk / 1024) << " KB | "
                  << static_cast<int>(mbps + 0.5) << " |" << std::endl;
    }
    return 0;
}

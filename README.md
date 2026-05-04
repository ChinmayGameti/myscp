```markdown
# myscp

A cross-platform (macOS/Linux) file transfer utility written in C++ using POSIX sockets and OpenSSL.

## Features
*   **Chunked Transfer:** Supports files up to 16GB+ without RAM exhaustion.
*   **Data Integrity:** SHA256 Checksum verification for every transfer.
*   **Cross-Platform:** Automatically handles Endianness differences between Apple Silicon (ARM) and Intel/AMD architectures.
*   **Fault Injection:** Built-in ability to intentionally corrupt checksums for testing error handling.

## Build Instructions
Ensure you have OpenSSL installed (`brew install openssl` on macOS or `sudo apt install libssl-dev` on Linux).
```bash
# Build the sender and receiver binaries
make
```

## Automated Testing
This repository includes a dedicated test environment with an automated script that creates a 5MB payload, transfers it over the local loopback, and verifies the checksums.
```bash
cd test
make test
```

## Manual Usage
To use the utility manually, start the receiver first, then run the sender.

### 1. Start Receiver
The receiver listens on port 9000 by default.
```bash
./receiver
```

### 2. Sender Commands
The sender binary requires the destination IP and the file path. It also supports an optional flag for fault injection testing.

**Syntax:**
```bash
./sender <destination_ip> <path_to_file> [--corrupt-checksum]
```

**Arguments:**
*   `<destination_ip>`: The IP address of the machine running the receiver (e.g., 127.0.0.1 for local testing).
*   `<path_to_file>`: The local path to the file you want to transfer.
*   `--corrupt-checksum`: (Optional) Intentionally flips bits in the SHA256 hash before transmission to trigger a verification failure on the receiver side.

**Examples:**
```bash
# Standard transfer to a local receiver
./sender 127.0.0.1 payload.bin

# Standard transfer to a remote Linux server
./sender 192.168.1.15 important_data.tar.gz

# Transfer with intentional checksum failure (Fault Injection)
./sender 127.0.0.1 payload.bin --corrupt-checksum
```

## Protocol Details
`myscp` uses a packed 300-byte binary header sent over raw TCP (`SOCK_STREAM`):
*   **Magic Number (4 Bytes):** Protocol identification (`0x53435058`).
*   **File Size (8 Bytes):** 64-bit integer (Network Byte Order).
*   **Filename (256 Bytes):** Null-terminated string.
*   **SHA256 Hash (32 Bytes):** Binary representation of the file hash.
```

# Project Description: MySCP Secure File Transfer

## 1. Architecture Overview
MySCP is a high-performance, cross-platform file transfer utility built in C++ that operates over a TCP-based SSH tunnel. It utilizes a Client-Server architecture invoked dynamically; the sender authenticates via Public Key, dynamically locates the receiver binary on the remote host, and streams data over a secure channel using a custom binary protocol.

### C4 Architecture Diagram
```mermaid
C4Context
    title System Context: MySCP Utility
    
    Person(user, "User", "Initiates file transfer")
    System(myscp, "MySCP", "Securely transfers large files up to 16GB")
    
    System_Ext(ssh_daemon, "OpenSSH Daemon", "Provides secure TCP transport layer")
    System_Ext(os_fs, "File System", "Source and Destination storage")

    Rel(user, myscp, "Executes myscp-send")
    Rel(myscp, ssh_daemon, "Establishes tunnel & spawns receiver")
    Rel(myscp, os_fs, "Reads/Writes 64KB Chunks")
```
## 2. Design Considerations

### Should I break the file into smaller chunks?
**Yes.** The utility reads and transmits data in `64 KB (65536 bytes)` chunks. This ensures that the memory footprint remains constant and incredibly small, allowing the application to securely transfer massive files (e.g., 16 GB+) without triggering Out-Of-Memory (OOM) killer processes on constrained systems. 

### Should I compress the file before transferring?
**No, but with a caveat.** Pre-compressing large files (like 16GB) requires massive upfront CPU cycles and temporary disk storage (Double I/O). Instead, MySCP delegates compression to the SSH transport layer (if configured via `~/.ssh/config`), allowing for real-time stream compression without altering the raw file bytes on disk.

### Error Handling and Retries
The system relies on strict boundary checks and POSIX error handling. 
- **Pre-flight Checks:** Verifies local file existence (`stat`) and remote binary availability before opening the data stream.
- **Network Interruptions:** The TCP/SSH layer handles packet loss. If the stream breaks fatally, the C++ `fread`/`fwrite` loops catch the broken pipe, terminate the transfer, and alert the user cleanly without segmentation faults.

### Economy of System Resource Utilization
The core philosophy of this tool is **Single-Pass Streaming**. 
Instead of hashing a 16GB file, transferring it, and hashing it again, MySCP calculates the SHA-256 hash *on the fly* as the 64KB chunks stream through the memory buffer. The final hash is appended to the stream as a `FileTrailer`. This strictly eliminates "Double I/O", cutting disk read operations by exactly 50%.

## 3. Performance Metrics

All numbers below are produced by `make bench` (see `bench/run_bench.sh`), which generates random payloads, transfers them via `myscp-send` to `127.0.0.1`, and captures wall-clock time and peak resident-set size with `/usr/bin/time`. Throughput is computed as `payload_bytes / wall_seconds`, expressed in MiB/s.

### Test Environment
- **Hardware:** M5, 12, 16GB, NVMe.
- **OS:** _macOS 14.5_
- **Compiler:** :C++
- **Network:** Loopback (`127.0.0.1`) — isolates CPU/IO performance from physical-link variance.

### Transfer Throughput vs File Size

| Size | Wall Time (s) | Throughput (MB/s) | Peak RSS (MB) |
| ---: | ---: | ---: | ---: |
| 100M | 0.52 |  192.3 | 10.1 |
|   1G | 0.92 | 1113.0 | 10.1 |
|   5G | 4.91 | 1042.8 | 10.0 |

> Reproduce with `SIZES="100M 1G 5G" make bench`; the table emitted to `bench/results.md` will overwrite the rows above.

### Chunk-Size Sensitivity

Output of `make bench-chunk` over a 1 GiB sample. This justifies the production `CHUNK_SIZE = 65536` selected in `common.h`: chunks below ~16 KB are dominated by syscall overhead, while chunks above ~256 KB show diminishing returns and inflate the peak working set without throughput gain.

| Chunk Size | Throughput (MB/s) |
| ---: | ---: |
|    4 KB | 1562 |
|   16 KB | 2640 |
|   64 KB | 2921 |
|  256 KB | 2931 |
| 1024 KB | 2919 |
| 4096 KB | 2899 |

### Interpretation

- **Constant memory footprint.** Peak RSS stays effectively flat across payload sizes because the sender and receiver both reuse a single 64 KB stack buffer per loop iteration; nothing in the hot path scales with file size.
- **Throughput is bounded by SHA-256 + SSH encryption**, not disk or syscall overhead, on loopback. On real networks throughput is bounded by link bandwidth long before the local CPU saturates.
- **Single-pass design pays off at scale.** A naive "hash, then send, then re-hash" pipeline would roughly double wall time on the multi-GiB rows because it would read the file from disk twice on the sender side.

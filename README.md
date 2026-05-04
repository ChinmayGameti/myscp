```markdown
# MySCP: High-Performance SSH Streaming Transfer

A robust, cross-platform C++ utility that streams files over secure SSH tunnels using a custom binary protocol. Built with **libssh** and **OpenSSL**, it features single-pass streaming to eliminate double I/O, on-the-fly SHA256 integrity verification, and automated environment bootstrapping.

## ✨ Key Features
- **Single-Pass Streaming:** Hashes and transmits data simultaneously (Header -> Data -> Trailer) to eliminate disk bottlenecking.
- **Automated Infrastructure:** Built-in OS detection automatically configures remote SSH daemons (handles Linux package managers and macOS TCC security gracefully).
- **Zero-Privilege Install:** Installs entirely in user-space (`~/.local/bin`). No `sudo` required for installation.
- **Smart Path Resolution:** Automatically hunts for the receiver binary across standard system paths on the remote host.

---

## 🚀 Quickstart

### 1. Install Dependencies
* **macOS:** `brew install libssh openssl@3`
* **Linux (Debian/Ubuntu):** `sudo apt install libssh-dev libssl-dev`

### 2. Build & Install
Run this on **both** your local machine and the remote machine:
```bash
make
make install
```
*(Note: The installer automatically grants execution permissions (`chmod +x`) to the binaries and will tell you if you need to add them to your `$PATH`).*

### 3. Bootstrap the Remote Environment
Run this on the **remote** machine to ensure the SSH daemon is ready to receive files:
```bash
make setup
```

### 4. Transfer a File
From your **local** machine, stream a file to the remote host:
```bash
myscp-send 192.168.1.50 my_large_file.zip
```

---

## 🛠 Command Reference

### Build & Setup Commands
| Command | Description |
| :--- | :--- |
| `make` | Compiles the `sender` and `receiver` binaries locally. |
| `make install` | Copies binaries to `~/.local/bin/` and applies `chmod +x` automatically. |
| `make setup` | Auto-detects your OS and configures the SSH daemon (`sshd`). |

### Testing Commands
If you are running the test scripts manually outside of the Makefile, ensure you grant them execution privileges first (e.g., `chmod +x test/run_test.sh`).

| Command | Description |
| :--- | :--- |
| `make run-test` | Runs the automated integration suite. Generates a payload, streams it over SSH `localhost`, and verifies it in a sandbox folder. |
| `make unit-test` | Compiles and executes the C++ assertion tests for header packing and endianness formatting. |

### Transfer Commands
| Command | Description |
| :--- | :--- |
| `myscp-send <ip> <file>` | Streams a file to the remote user's home directory. |
| `myscp-send <ip> <file> <remote/path/>` | Streams a file to a specific remote destination. |
| `myscp-send <ip> --setup` | Performs a pre-flight diagnostic to verify SSH keys and remote binary availability without sending data. |
| `myscp-send <ip> <file> --corrupt-checksum` | Intentionally flips a bit in the SHA256 trailer to test the receiver's rejection logic. |

### Cleanup Commands
| Command | Description |
| :--- | :--- |
| `make clean` | Wipes the local repository of compiled binaries, `.o` files, macOS `.dSYM` folders, and all test sandbox payloads. |
| `make uninstall` | Removes the installed `myscp-send` and `myscp-recv` binaries from your system's `~/.local/bin/` folder. Always run this when completely removing the project. |

---

## 🔍 Debugging & Architecture

If you are running into "Connection Refused" or "Access Denied" errors, or if you want to understand the system architecture, check your SSH configuration.

**Resolving Authentication Failures:**
`myscp` relies on Public Key Authentication to execute the remote receiver silently. If prompted for a password, push your key to the server:
```bash
ssh-copy-id user@192.168.1.50
```

**Manual Daemon Debugging (macOS/Linux):**
If you need to bypass system services and run the SSH daemon manually in the foreground so it accepts multiple connections indefinitely, use:
```bash
sudo /usr/sbin/sshd -D -p 22
```

*Deep Debugging:* If you want to watch the real-time cryptographic handshakes to debug a specific connection issue, add the `-d` flag. **Note: In `-d` (debug) mode, the server will intentionally exit after handling exactly one connection.**
```bash
sudo /usr/sbin/sshd -D -d -p 22
```

```

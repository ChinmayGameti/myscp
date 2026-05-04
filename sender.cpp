#include "common.h"
#include <libssh/libssh.h>

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
        std::cerr << "Usage: " << argv[0] << " <ip> <local_file> [remote_path] [--corrupt-checksum]" << std::endl;
        return 1;
    }

    bool inject_fault = false;
    std::string remote_path = "";

    // Parse arguments
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--corrupt-checksum") == 0) {
            inject_fault = true;
        } else {
            remote_path = argv[i];
        }
    }

    struct stat st;
    if (stat(argv[2], &st) != 0) {
        perror("[ERROR] Local file stat failed");
        return 1;
    }

    uint8_t hash[32];
    calculate_sha256(argv[2], hash);
    if (inject_fault) {
        std::cout << "[DEBUG] Injecting intentional checksum fault..." << std::endl;
        hash[0] ^= 0xFF; 
    }

    ssh_session my_ssh_session = ssh_new();
    if (my_ssh_session == NULL) exit(-1);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, argv[1]);
    
    std::cout << "[INFO] Connecting to " << argv[1] << "..." << std::endl;
    if (ssh_connect(my_ssh_session) != SSH_OK) {
        std::cerr << "[ERROR] Connection failed: " << ssh_get_error(my_ssh_session) << std::endl;
        return 1;
    }

    if (ssh_userauth_publickey_auto(my_ssh_session, NULL, NULL) != SSH_AUTH_SUCCESS) {
        std::cerr << "[ERROR] Authentication failed: " << ssh_get_error(my_ssh_session) << std::endl;
        return 1;
    }

    ssh_channel channel = ssh_channel_new(my_ssh_session);
    if (channel == NULL || ssh_channel_open_session(channel) != SSH_OK) return 1;

    std::string exec_cmd = "~/.local/bin/myscp-recv --server";
    if (!remote_path.empty()) exec_cmd += " " + remote_path;

    if (ssh_channel_request_exec(channel, exec_cmd.c_str()) != SSH_OK) {
        std::cerr << "[ERROR] Cannot execute remote receiver." << std::endl;
        return 1;
    }

    FileHeader header = {}; 
    header.magic = htonl(MAGIC_NUMBER);
    header.file_size = htobe64(st.st_size);
    strncpy(header.filename, basename((char*)argv[2]), 256);
    memcpy(header.checksum, hash, 32);

    ssh_channel_write(channel, &header, sizeof(FileHeader));

    FILE* fp = fopen(argv[2], "rb");
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    bool transmission_success = true;

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        // If receiver crashes or exits (e.g., path not found), write will fail
        int written = ssh_channel_write(channel, buffer, bytes_read);
        if (written < 0) {
            std::cerr << "❌ [ERROR] Connection lost or receiver terminated early." << std::endl;
            transmission_success = false;
            break;
        }
    }
    fclose(fp);
    
    if (transmission_success) {
        ssh_channel_send_eof(channel);
    }

    // Read any output (success or errors) thrown by the remote receiver
    std::cout << "\n--- Remote Server Logs ---" << std::endl;
    char remote_output[256];
    int nbytes;
    while ((nbytes = ssh_channel_read(channel, remote_output, sizeof(remote_output), 0)) > 0) {
        std::cout.write(remote_output, nbytes);
    }
    std::cout << "--------------------------" << std::endl;

    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    
    return transmission_success ? 0 : 1;
}

#include "common.h"
#include <libssh/libssh.h>

int remote_check(ssh_session session, const std::string& cmd) {
    ssh_channel channel = ssh_channel_new(session);
    if (!channel) return SSH_ERROR;
    if (ssh_channel_open_session(channel) != SSH_OK) return SSH_ERROR;
    
    int rc = ssh_channel_request_exec(channel, cmd.c_str());
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return rc;
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ip> [file] [remote_path] [--setup] [--corrupt-checksum]" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    std::string local_file = "";
    std::string remote_path = "";
    bool run_setup = false;
    bool inject_fault = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--setup") == 0) run_setup = true;
        else if (strcmp(argv[i], "--corrupt-checksum") == 0) inject_fault = true;
        else if (local_file.empty()) local_file = argv[i];
        else if (remote_path.empty()) remote_path = argv[i];
    }

    ssh_session my_ssh_session = ssh_new();
    if (my_ssh_session == NULL) exit(-1);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, ip.c_str());

    std::cout << "[INFO] Connecting to " << ip << "..." << std::endl;
    if (ssh_connect(my_ssh_session) != SSH_OK) {
        std::cerr << "❌ [ERROR] Connection failed: " << ssh_get_error(my_ssh_session) << std::endl;
        ssh_free(my_ssh_session);
        return 1;
    }

    if (ssh_userauth_publickey_auto(my_ssh_session, NULL, NULL) != SSH_AUTH_SUCCESS) {
        std::cerr << "❌ [ERROR] Authentication failed. Try: ssh-copy-id " << ip << std::endl;
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return 1;
    }

    if (run_setup) {
        std::cout << "--- Environment Diagnostic ---" << std::endl;
        if (remote_check(my_ssh_session, "test -f ~/.local/bin/myscp-recv") == SSH_OK)
            std::cout << "✅ Remote binary found at ~/.local/bin/myscp-recv" << std::endl;
        else
            std::cout << "⚠️  Remote binary MISSING at ~/.local/bin." << std::endl;
        
        std::cout << "✅ SSH Key authentication working." << std::endl;
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return 0;
    }

    if (local_file.empty()) {
        std::cerr << "[ERROR] No file specified." << std::endl;
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return 1;
    }

    struct stat st;
    if (stat(local_file.c_str(), &st) != 0) {
        perror("❌ [ERROR] Local file error (Does the file exist?)");
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        return 1;
    }

    ssh_channel channel = ssh_channel_new(my_ssh_session);
    if (channel == NULL || ssh_channel_open_session(channel) != SSH_OK) return 1;

    // FIX: Safely construct the arguments first
    std::string args = " --server";
    if (!remote_path.empty()) {
        args += " '" + remote_path + "'"; // Wrap path in quotes to handle spaces safely
    }

    // FIX: Inject arguments INSIDE the bash conditions
    std::string exec_cmd = 
        "if [ -f ~/.local/bin/myscp-recv ]; then ~/.local/bin/myscp-recv" + args + "; "
        "elif [ -f /usr/local/bin/myscp-recv ]; then /usr/local/bin/myscp-recv" + args + "; "
        "else myscp-recv" + args + "; fi";

    if (ssh_channel_request_exec(channel, exec_cmd.c_str()) != SSH_OK) {
        std::cerr << "❌ [ERROR] Could not launch remote receiver." << std::endl;
        return 1;
    }

    FileHeader header = {}; 
    header.magic = htonl(MAGIC_NUMBER);
    header.file_size = htobe64(st.st_size);
    
    std::string base_name = local_file;
    size_t pos = base_name.find_last_of("/\\");
    if (pos != std::string::npos) base_name = base_name.substr(pos + 1);
    strncpy(header.filename, base_name.c_str(), sizeof(header.filename) - 1);
    header.filename[sizeof(header.filename) - 1] = '\0';

    ssh_channel_write(channel, &header, sizeof(FileHeader));

    FILE* fp = fopen(local_file.c_str(), "rb");
    if (!fp) {
        std::cerr << "❌ [ERROR] Could not open local file for reading." << std::endl;
        return 1;
    }

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    bool success = true;

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        SHA256_Update(&sha256, buffer, bytes_read);
        if (ssh_channel_write(channel, buffer, bytes_read) < 0) {
            std::cerr << "❌ [ERROR] Transmission interrupted." << std::endl;
            success = false;
            break;
        }
    }
    fclose(fp);

    if (success) {
        FileTrailer trailer;
        SHA256_Final(trailer.checksum, &sha256);
        if (inject_fault) trailer.checksum[0] ^= 0xFF; 
        
        ssh_channel_write(channel, &trailer, sizeof(FileTrailer));
        ssh_channel_send_eof(channel);
    }

    std::cout << "\n--- Remote Logs ---" << std::endl;
    char remote_out[256];
    int nbytes;
    while ((nbytes = ssh_channel_read(channel, remote_out, sizeof(remote_out), 0)) > 0) {
        std::cout.write(remote_out, nbytes);
    }
    std::cout << "-------------------" << std::endl;

    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    
    return success ? 0 : 1;
}

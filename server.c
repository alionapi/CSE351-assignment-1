#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "vigenere.h"

#define MAX_MESSAGE_SIZE 10000000  // is allowed 10 MB
#define HEADER_SIZE 8  // total 2 + 2 + 4 bytes
#define MAX_CONNECTIONS 50
typedef struct {
    uint16_t op;
    uint16_t key_length;
    uint32_t data_length;
} message_header_t;
int active_connections = 0;
int send_all(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char *)buf;
    while (total_sent < len) {
        ssize_t sent = send(sockfd, ptr + total_sent, len - total_sent, 0);
        if (sent == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_sent += sent;
    }
    return 0;
}
int recv_all(int sockfd, void *buf, size_t len) {
    size_t total_received = 0;
    char *ptr = (char *)buf;
    while (total_received < len) {
        ssize_t received = recv(sockfd, ptr + total_received, len - total_received, 0);
        if (received == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (received == 0) return -1;  
        total_received += received;
    }
    return 0;
}
int handle_client_message(int client_fd) {
    message_header_t header;
    // Receiving this header
    if (recv_all(client_fd, &header, HEADER_SIZE) == -1) {
        return -1;
    }
    // Convert from network byte order here
    uint16_t op = ntohs(header.op);
    uint16_t key_length = ntohs(header.key_length);
    uint32_t data_length = ntohl(header.data_length);
    // Validate protocol now
    if (op > 1 || key_length == 0 || data_length == 0) {
        return -1;
    }
    // Check message size limit
    size_t total_size = HEADER_SIZE + key_length + data_length;
    if (total_size > MAX_MESSAGE_SIZE) {
        return -1;
    }
    // Allocate buffers
    char *key = malloc(key_length + 1);
    char *data = malloc(data_length);
    char *result = malloc(data_length + 1);
    if (!key || !data || !result) {
        free(key);
        free(data);
        free(result);
        return -1;
    }
    // Receive this key
    if (recv_all(client_fd, key, key_length) == -1) {
        free(key);
        free(data);
        free(result);
        return -1;
    }
    key[key_length] = '\0';
    // Receive data
    if (recv_all(client_fd, data, data_length) == -1) {
        free(key);
        free(data);
        free(result);
        return -1;
    }
    // Perform Vigenère cipher
    if (op == 0) {
        // Encrypt
        vigenere_encrypt(data, key, result, data_length);
    } else {
        // Decrypt
        vigenere_decrypt(data, key, result, data_length);
    }
    // Prepare response header
    message_header_t resp_header;
    resp_header.op = htons(op);
    resp_header.key_length = htons(key_length);
    resp_header.data_length = htonl(data_length);
    // Send response
    int success = 0;
    if (send_all(client_fd, &resp_header, HEADER_SIZE) == 0 &&
        send_all(client_fd, key, key_length) == 0 &&
        send_all(client_fd, result, data_length) == 0) {
        success = 1;
    }
    free(key);
    free(data);
    free(result);
    
    return success ? 0 : -1;
}
void handle_client(int client_fd) {
    while (1) {
        if (handle_client_message(client_fd) == -1) {
            break;
        }
    }
    close(client_fd);
    exit(0);
}
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        active_connections--;
    }
}
int main(int argc, char *argv[]) {
    int port = 0;
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
            break;
        }
    }
    if (port <= 0) {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return 1;
    }
    // Setup signal handler for child processes
    signal(SIGCHLD, sigchld_handler);
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    // Listen for connections
    if (listen(server_fd, MAX_CONNECTIONS) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    fprintf(stderr, "Server listening on port %d\n", port);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        // Check connection limit
        if (active_connections >= MAX_CONNECTIONS) {
            close(client_fd);
            continue;
        }
        // Fork child process to handle client
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_fd);
            handle_client(client_fd);
        } else if (pid > 0) {
            // Parent process
            close(client_fd);
            active_connections++;
        } else {
            perror("fork");
            close(client_fd);
        }
    }
    close(server_fd);
    return 0;
}

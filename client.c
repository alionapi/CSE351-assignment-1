#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>


#define MAX_MESSAGE_SIZE 10000000  // 10 MB
#define HEADER_SIZE 8  // 2 + 2 + 4 bytes
typedef struct {
    uint16_t op;
    uint16_t key_length;
    uint32_t data_length;
} message_header_t;
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
        if (received == 0) return -1;  // Connection closed
        total_received += received;
    }
    return 0;
}
int process_data_chunk(int sockfd, const char *data, size_t data_len, const char *key, int op) {
    size_t key_len = strlen(key);
    if (HEADER_SIZE + key_len + data_len > MAX_MESSAGE_SIZE) {
        fprintf(stderr, "Chunk too large for protocol\n");
        return -1;
    }
    message_header_t header;
    header.op = htons((uint16_t)op);
    header.key_length = htons((uint16_t)key_len);
    header.data_length = htonl((uint32_t)data_len);
    // Send header
    if (send_all(sockfd, &header, HEADER_SIZE) == -1) {
        return -1;
    }
    // Send key
    if (send_all(sockfd, key, key_len) == -1) {
        return -1;
    }
    // Send data chunk
    if (send_all(sockfd, data, data_len) == -1) {
        return -1;
    }
    // Receive response header
    message_header_t resp_header;
    if (recv_all(sockfd, &resp_header, HEADER_SIZE) == -1) {
        return -1;
    }
    uint16_t resp_op = ntohs(resp_header.op);
    uint16_t resp_key_len = ntohs(resp_header.key_length);
    uint32_t resp_data_len = ntohl(resp_header.data_length);
    // Validate response
    if (resp_op != op || resp_key_len != key_len) {
        fprintf(stderr, "Invalid response header\n");
        return -1;
    }
    // Skip key in response
    char *temp_key = malloc(resp_key_len);
    if (!temp_key) {
        return -1;
    }
    if (recv_all(sockfd, temp_key, resp_key_len) == -1) {
        free(temp_key);
        return -1;
    }
    free(temp_key);
    // Receive and output response data
    char *resp_data = malloc(resp_data_len);
    if (!resp_data) {
        return -1;
    }
    if (recv_all(sockfd, resp_data, resp_data_len) == -1) {
        free(resp_data);
        return -1;
    }
    // Output response data directly to stdout (no newline)
    if (fwrite(resp_data, 1, resp_data_len, stdout) != resp_data_len) {
        free(resp_data);
        return -1;
    }
    fflush(stdout);
    
    free(resp_data);
    return 0;
}
int process_all_data(int sockfd, const char *data, size_t data_len, const char *key, int op) {
    size_t key_len = strlen(key);
    size_t max_chunk_size = MAX_MESSAGE_SIZE - HEADER_SIZE - key_len;
    
    // Handle case where key is too long
    if (max_chunk_size <= 0) {
        fprintf(stderr, "Key too long for protocol\n");
        return -1;
    }
    size_t offset = 0;
    while (offset < data_len) {
        size_t chunk_size = data_len - offset;
        if (chunk_size > max_chunk_size) {
            chunk_size = max_chunk_size;
        }
        if (process_data_chunk(sockfd, data + offset, chunk_size, key, op) == -1) {
            return -1;
        }
        offset += chunk_size;
    }
    
    return 0;
}
char *read_stdin_data(size_t *data_length) {
    char *input_data = NULL;
    size_t input_capacity = 8192;  // Start with 8KB buffer
    size_t input_length = 0;
    input_data = malloc(input_capacity);
    if (!input_data) {
        return NULL;
    }
    // Read all data from stdin byte by byte to handle binary data correctly
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        // Expand buffer if needed
        if (input_length >= input_capacity) {
            size_t new_capacity = input_capacity * 2;
            char *new_data = realloc(input_data, new_capacity);
            if (!new_data) {
                free(input_data);
                return NULL;
            }
            input_data = new_data;
            input_capacity = new_capacity;
        }
        input_data[input_length++] = (char)c;
    }
    *data_length = input_length;
    return input_data;
}
int main(int argc, char *argv[]) {
    char *host = NULL;
    int port = 0;
    int op = 0;
    char *key = NULL;
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            host = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            op = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            key = argv[++i];
        }
    }
    // Validate arguments
    if (!host || port <= 0 || !key || (op != 0 && op != 1)) {
        fprintf(stderr, "Usage: %s -h <host> -p <port> -o <operation> -k <key>\n", argv[0]);
        fprintf(stderr, "operation: 0 for encrypt, 1 for decrypt\n");
        return 1;
    }
    // Validate key length for protocol
    size_t key_len = strlen(key);
    if (key_len == 0 || key_len > 65535) {
        fprintf(stderr, "Key length must be between 1 and 65535 characters\n");
        return 1;
    }
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return 1;
    }
    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return 1;
    }
    // Read all data from stdin
    size_t input_length;
    char *input_data = read_stdin_data(&input_length);
    if (!input_data) {
        fprintf(stderr, "Failed to read stdin data\n");
        close(sockfd);
        return 1;
    }
    // Process data (may involve multiple messages if large)
    int result = 0;
    if (input_length > 0) {
        if (process_all_data(sockfd, input_data, input_length, key, op) == -1) {
            result = 1;
        }
    }
    free(input_data);
    close(sockfd);
    return result;
}

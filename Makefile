CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
LDFLAGS = 

# Source files
CLIENT_SOURCES = client.c
SERVER_SOURCES = server.c vigenere.c
COMMON_HEADERS = vigenere.h

# Executable names
CLIENT_TARGET = client
SERVER_TARGET = server

# Default target
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Build client
$(CLIENT_TARGET): $(CLIENT_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build server
$(SERVER_TARGET): $(SERVER_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)

# Phony targets
.PHONY: all clean

# Dependencies
client.o: client.c
server.o: server.c $(COMMON_HEADERS)
vigenere.o: vigenere.c $(COMMON_HEADERS)
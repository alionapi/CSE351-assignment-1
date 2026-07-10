# CSE351-assignment-1
CSE351: Computer Networks | Fall 2025 | 

Programming Assignment 1

TCP Client-Server Application with Vigenère Cipher

A TCP client-server application written in C that performs Vigenère cipher encryption and decryption using a custom application-layer protocol.

The client sends requests containing an operation type, key, and data. The server processes the request and returns the resulting ciphertext or plaintext.

## Features

* TCP socket communication
* Custom binary application-layer protocol
* Vigenère cipher encryption and decryption
* Network byte order handling
* Support for arbitrary input data
* Concurrent client handling
* Large message support through chunking
* Build automation using Makefile

## Repository Structure

```text
.
├── CSE351_PA1.pdf     # Assignment specification
├── Makefile           # Build configuration
├── README.md          # Repository documentation
├── client.c           # Client implementation
├── server.c           # Server implementation
├── vigenere.c         # Vigenère cipher implementation
├── vigenere.h         # Vigenère cipher declarations

```

## Protocol Format

Each request contains a fixed-size header followed by variable-length key and data fields.

```text
+------------+-------------+--------------+----------------------+
| op (16bit) | key_length  | data_length  | key_string || data  |
|            |  (16bit)    |   (32bit)    |                      |
+------------+-------------+--------------+----------------------+
```

### Header Fields

| Field       | Size    | Description                               |
| ----------- | ------- | ----------------------------------------- |
| op          | 16 bits | Operation type (0 = encrypt, 1 = decrypt) |
| key_length  | 16 bits | Length of key_string in bytes             |
| data_length | 32 bits | Length of data_string in bytes            |

* `key_length` and `data_length` are transmitted in network byte order.
* The payload is stored as `key_string || data_string`.
* Maximum message size is 10,000,000 bytes.

## Building

Compile the project with:

```bash
make all
```

This produces the following executables:

```text
client
server
```

## Running the Server

Start the server on a specified port:

```bash
./server -p 1234
```

## Running the Client

Run the client with the server address, port, operation type, and key:

```bash
./client -h 127.0.0.1 -p 1234 -o 0 -k uailab
```

### Arguments

| Option | Description                               |
| ------ | ----------------------------------------- |
| -h     | Server IP address                         |
| -p     | Server port number                        |
| -o     | Operation type (0 = encrypt, 1 = decrypt) |
| -k     | Vigenère cipher key                       |

The client reads input from standard input, sends the request to the server, receives the processed result, and prints the response to standard output.

Example:

```bash
echo "Hello World" | ./client -h 127.0.0.1 -p 1234 -o 0 -k uailab
```

## Vigenère Cipher Behavior

* Uppercase letters are converted to lowercase.
* Encryption and decryption are applied only to alphabetic characters.
* Non-alphabetic characters remain unchanged.
* The key is repeated as necessary across the input string.

## Source Files

### client.c

Implements the TCP client, including:

* Command-line argument parsing
* Input handling
* Message construction
* Request transmission
* Response reception

### server.c

Implements the TCP server, including:

* Socket creation and binding
* Connection handling
* Protocol validation
* Request processing
* Response generation
* Concurrent client support

### vigenere.c / vigenere.h

Provide the Vigenère cipher implementation used by the server for encryption and decryption operations.


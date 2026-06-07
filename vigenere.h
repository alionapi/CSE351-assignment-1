#ifndef VIGENERE_H
#define VIGENERE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void vigenere_encrypt(const char *plaintext, const char *key, char *ciphertext, size_t text_len);
void vigenere_decrypt(const char *ciphertext, const char *key, char *plaintext, size_t text_len);

#endif
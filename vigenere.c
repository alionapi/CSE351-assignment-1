#include "vigenere.h"

void vigenere_encrypt(const char *plaintext, const char *key, char *ciphertext, size_t text_len) {
    size_t key_len = strlen(key);
    size_t key_index = 0;
    
    for (size_t i = 0; i < text_len; i++) {
        char c = plaintext[i];
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        if (c >= 'a' && c <= 'z') {
            // Get here a key character and convert to lowercase
            char key_char = key[key_index % key_len];
            if (key_char >= 'A' && key_char <= 'Z') {
                key_char = key_char - 'A' + 'a';
            }
            // Apply Vigenère cipher that follows the formula (plaintext + key) mod 26
            int shift = key_char - 'a';
            ciphertext[i] = ((c - 'a' + shift) % 26) + 'a';
            key_index++;
        } else {
            // Non-alphabetic characters will remain unchanged
            ciphertext[i] = plaintext[i];
        }
    }
}
void vigenere_decrypt(const char *ciphertext, const char *key, char *plaintext, size_t text_len) {
    size_t key_len = strlen(key);
    size_t key_index = 0;
    for (size_t i = 0; i < text_len; i++) {
        char c = ciphertext[i];
        // Check if character is alphabetic and convert to lowercase
        if (isalpha(c)) {
            c = tolower(c);
            // Get key character and convert to lowercase
            char key_char = key[key_index % key_len];
            key_char = tolower(key_char);
            // Apply Vigenère decipher: (ciphertext - key + 26) mod 26
            int shift = key_char - 'a';
            int decrypted_value = (c - 'a' - shift + 26) % 26;
            plaintext[i] = decrypted_value + 'a';
            key_index++;
        } else {
            // Non-alphabetic characters remain unchanged
            plaintext[i] = ciphertext[i];
        }
    }
}

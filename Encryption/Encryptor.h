#ifndef AES_ENCRYPTOR_H
#define AES_ENCRYPTOR_H

#include <openssl/evp.h>
#include <vector>
#include <string>

using namespace std;


class AES_Encryptor {
private:
    vector<unsigned char> key; // 32 bytes for AES-256 means have 256 bits
    vector<unsigned char> iv;  // 16 bytes for AES block size means 128 block size for the Encryptor

public:
    AES_Encryptor(const vector<unsigned char>& key, const vector<unsigned char>& iv)
        : key(key), iv(iv) {}

    vector<unsigned char> encrypt(const vector<unsigned char>& plaintext);
   
    vector<unsigned char> decrypt(const vector<unsigned char>& ciphertext);

    // Helper functions for files
    static vector<unsigned char> read_file(const string& filename);
   
    static void write_file(const string& filename, const vector<unsigned char>& data);
};

#endif

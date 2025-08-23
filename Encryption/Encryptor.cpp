#include "Encryptor.h"
#include <fstream>
#include <iostream>

using namespace std;

// AES Encryptor basically is Encryting the data by using a defined key size which will be defined whenit will be used

 vector<unsigned char> AES_Encryptor::encrypt(const  vector<unsigned char>& plaintext) {
     vector<unsigned char> ciphertext(plaintext.size() + 16);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len;

    // We are using the AES 256 Means the key will have 256 bits and there will be 14 Rounds of encryption
    // Intial transformation
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data());

    // Till N-1 Rounds
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size());
    int total_len = len;

    // the final Round
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    total_len += len;
    ciphertext.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext;
}

// This is the Decrytor for the AES_Encryptor class 
// As the Aes is a Block Ciper so its Decryption Method will be Just Opposite to Encrytption

 vector<unsigned char> AES_Encryptor::decrypt(const  vector<unsigned char>& ciphertext) {
     vector<unsigned char> plaintext(ciphertext.size());
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len;
    // Initial Round
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data());
    
    // Tille the second last round
    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());
    int total_len = len;
   
    //    the last Round
    EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    total_len += len;
    plaintext.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);
    return plaintext;
}

// This function is used to read the file in binary format so it will be easier for the Encrptor to Encrpt

 vector<unsigned char> AES_Encryptor::read_file(const  string& filename) {
    
    ifstream file(filename,  ios::binary);
    
     if(!file) {  cerr << "Failed to open file: " << filename << "\n"; return {}; }
     vector<unsigned char> buffer(( istreambuf_iterator<char>(file)),
                                        istreambuf_iterator<char>());
    return buffer;
}

// This is used to write the binary file we got from the sender to deccrpt and convert into real file

void AES_Encryptor::write_file(const  string& filename, const  vector<unsigned char>& data) {
     
    ofstream file(filename,  ios::binary);
   
     if(!file) {  cerr << "Failed to write file: " << filename << "\n"; return; }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

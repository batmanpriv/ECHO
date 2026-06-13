#ifndef CRYPTO_H
#define CRYPTO_H

#include <cstring>
#include <ctime>
#include <random>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"

class Crypto {
private:
    unsigned char key[32];
    unsigned char iv[16];
    unsigned int counter;
    
    unsigned char s_box[256] = {
        0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
        0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
        0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
        0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
        0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
        0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
        0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
        0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
        0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
        0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
        0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
        0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
        0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
        0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
        0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
        0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    };
    
    unsigned char inv_s_box[256];
    
    void generate_inv_sbox() {
        memset(inv_s_box, 0, 256);
        for (int i = 0; i < 256; i++) {
            inv_s_box[s_box[i]] = i;
        }
    }
    
    unsigned int rotl32(unsigned int x, int shift) {
        return (x << shift) | (x >> (32 - shift));
    }
    
    void chacha20_quarter_round(unsigned int& a, unsigned int& b, unsigned int& c, unsigned int& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }
    
void generate_keystream(unsigned char* keystream, int block_num) {
    unsigned int state[16];
    
    state[0] = 0x61707865;
    state[1] = 0x3320646E;
    state[2] = 0x79622D32;
    state[3] = 0x6B206574;
    
    for (int i = 0; i < 8; i++) {
        state[4 + i] = ((unsigned int)key[i*4]) |
                       ((unsigned int)key[i*4+1] << 8) |
                       ((unsigned int)key[i*4+2] << 16) |
                       ((unsigned int)key[i*4+3] << 24);
    }
    
    state[12] = counter + block_num;
    state[13] = 0;
    
    for (int i = 0; i < 4; i++) {
        state[14 + i] = ((unsigned int)iv[i*4]) |
                        ((unsigned int)iv[i*4+1] << 8) |
                        ((unsigned int)iv[i*4+2] << 16) |
                        ((unsigned int)iv[i*4+3] << 24);
    }
    
    unsigned int working[16];
    memcpy(working, state, 64);
    
    for (int round = 0; round < 10; round++) {
        chacha20_quarter_round(working[0], working[4], working[8], working[12]);
        chacha20_quarter_round(working[1], working[5], working[9], working[13]);
        chacha20_quarter_round(working[2], working[6], working[10], working[14]);
        chacha20_quarter_round(working[3], working[7], working[11], working[15]);
        chacha20_quarter_round(working[0], working[5], working[10], working[15]);
        chacha20_quarter_round(working[1], working[6], working[11], working[12]);
        chacha20_quarter_round(working[2], working[7], working[8], working[13]);
        chacha20_quarter_round(working[3], working[4], working[9], working[14]);
    }
    
    for (int i = 0; i < 16; i++) {
        working[i] += state[i];
    }
    
    for (int i = 0; i < 16; i++) {
        keystream[i*4] = working[i] & 0xFF;
        keystream[i*4+1] = (working[i] >> 8) & 0xFF;
        keystream[i*4+2] = (working[i] >> 16) & 0xFF;
        keystream[i*4+3] = (working[i] >> 24) & 0xFF;
    }
}

#pragma GCC diagnostic pop
    
public:
    Crypto() {
        srand(time(NULL));
        for (int i = 0; i < 32; i++) {
            key[i] = rand() % 256;
        }
        for (int i = 0; i < 16; i++) {
            iv[i] = rand() % 256;
        }
        counter = 0;
        generate_inv_sbox();
    }
    
    Crypto(const unsigned char* pre_shared_key, int key_len) {
        for (int i = 0; i < 32; i++) {
            key[i] = pre_shared_key[i % key_len];
            key[i] ^= (i * 0x9E3779B9) & 0xFF;
            key[i] ^= (pre_shared_key[(i + 13) % key_len] * 0x1B) & 0xFF;
            key[i] = s_box[key[i]];
            key[i] ^= (pre_shared_key[(i + 7) % key_len] >> 1) & 0xFF;
        }
        
        for (int i = 0; i < 16; i++) {
            iv[i] = key[i] ^ key[i + 16];
            iv[i] = s_box[iv[i]];
        }
        
        counter = 0;
        generate_inv_sbox();
    }

    void reset() {
        counter = 0;
        
        for (int i = 0; i < 16; i++) {
            iv[i] = key[i] ^ key[i + 16];
            iv[i] = s_box[iv[i]];
        }
    }
void encrypt(unsigned char* data, int len) {
    unsigned char keystream[64];
    
    for (int offset = 0; offset < len; offset += 64) {
        generate_keystream(keystream, offset / 64);
        
        int block_size = (len - offset < 64) ? len - offset : 64;
        for (int i = 0; i < block_size; i++) {
            data[offset + i] ^= keystream[i]; 
            data[offset + i] = s_box[data[offset + i]]; 
        }
    }
    
    counter++;
}

void decrypt(unsigned char* data, int len) {
    unsigned char keystream[64];
    
    for (int offset = 0; offset < len; offset += 64) {
        generate_keystream(keystream, offset / 64);
        
        int block_size = (len - offset < 64) ? len - offset : 64;
        for (int i = 0; i < block_size; i++) {
            data[offset + i] = inv_s_box[data[offset + i]];  
            data[offset + i] ^= keystream[i];  
        }
    }
    
    counter++;
}
};

#endif

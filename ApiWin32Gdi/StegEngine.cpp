#include "StegEngine.h"
#include <cstring>

static const BYTE MAGIC[4] = { 'S','T','E','G' };

// On écrit les bits dans l’ordre BGRA (on n’utilise pas A), ici B,G,R => 3 LSB par pixel.
static inline void put_bit(BYTE& b, int bit) {
    b = (BYTE)((b & 0xFE) | (bit & 1));
}

static inline int get_bit(BYTE b) {
    return (b & 1);
}

bool EmbedLSB(BYTE* pixels, size_t size, const std::string& msg) {
    if (!pixels) return false;

    // Buffer total à écrire
    const uint32_t len = (uint32_t)msg.size();
    const size_t total = 4 + 4 + (size_t)len;

    // Capacité : nombre de pixels * 3 bits (B,G,R)
    const size_t pixelsCount = size / 4;
    const size_t capacityBits = pixelsCount * 3;
    const size_t neededBits = total * 8ull;
    if (neededBits > capacityBits) return false;

    // Index bit courant dans le flux d'écriture (0..capacityBits-1)
    size_t bitIndex = 0;

    auto writeByte = [&](BYTE v) {
        for (int i = 0; i < 8; ++i) {
            int bit = (v >> i) & 1;
            size_t p = bitIndex / 3;
            int c = (int)(bitIndex % 3);
            BYTE* px = pixels + p * 4;
            if (c == 0) put_bit(px[0], bit);
            else if (c == 1) put_bit(px[1], bit);
            else put_bit(px[2], bit);
            ++bitIndex;
        }
        };

    // Ecriture de MAGIC, longueur (little endian) puis données
    for (int i = 0; i < 4; ++i) writeByte(MAGIC[i]);
    writeByte((BYTE)((len) & 0xFF));
    writeByte((BYTE)((len >> 8) & 0xFF));
    writeByte((BYTE)((len >> 16) & 0xFF));
    writeByte((BYTE)((len >> 24) & 0xFF));
    for (size_t i = 0; i < len; ++i) writeByte((BYTE)msg[i]);

    return true;
}

std::string ExtractLSB(const BYTE* pixels, size_t size) {
    std::string out;
    if (!pixels) return out;

    const size_t pixelsCount = size / 4;
    const size_t capacityBits = pixelsCount * 3;

    auto readByteAt = [&](size_t& bitIndex) -> BYTE {
        BYTE v = 0;
        for (int i = 0; i < 8; ++i) {
            if (bitIndex >= capacityBits) return 0;
            size_t p = bitIndex / 3;
            int c = (int)(bitIndex % 3);
            const BYTE* px = pixels + p * 4;
            int bit = (c == 0) ? get_bit(px[0]) : (c == 1) ? get_bit(px[1]) : get_bit(px[2]);
            v |= (BYTE)(bit << i);  // LSB-first
            ++bitIndex;
        }
        return v;
        };

    size_t bitIndex = 0;

    // Lire MAGIC (4 octets) et vérifier
    BYTE m[4];
    for (int i = 0; i < 4; ++i) m[i] = readByteAt(bitIndex);
    if (!(m[0] == MAGIC[0] && m[1] == MAGIC[1] && m[2] == MAGIC[2] && m[3] == MAGIC[3])) {
        return out; // signature non trouvée -> pas de message
    }

    // Lire la longueur (little endian)
    uint32_t len = 0;
    len |= (uint32_t)readByteAt(bitIndex) << 0;
    len |= (uint32_t)readByteAt(bitIndex) << 8;
    len |= (uint32_t)readByteAt(bitIndex) << 16;
    len |= (uint32_t)readByteAt(bitIndex) << 24;

    // Sécurité : vérifier qu'il reste assez de bits pour lire len octets
    size_t neededBits = (size_t)len * 8ull;
    if (bitIndex + neededBits > capacityBits) return std::string();

    out.resize(len);
    for (uint32_t i = 0; i < len; ++i) out[i] = (char)readByteAt(bitIndex);
    return out;
}
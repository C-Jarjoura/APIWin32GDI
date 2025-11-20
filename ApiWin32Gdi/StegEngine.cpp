#include "StegEngine.h"
#include <cstring>

// Magic signature pour identifier la présence d'un message dans le flux de LSB.
// On écrit d'abord ces 4 octets 'S','T','E','G'.
static const BYTE MAGIC[4] = { 'S','T','E','G' };

// Ecriture/lecture d'un bit dans le LSB d'un octet.
// On n'altère que le bit de poids faible du canal, les autres bits restent inchangés.
static inline void put_bit(BYTE& b, int bit) {
    b = (BYTE)((b & 0xFE) | (bit & 1));
}

static inline int get_bit(BYTE b) {
    return (b & 1);
}

// EmbedLSB : encode messageUtf8 dans pixels (BGRA 32bpp).
// Format stocké : [MAGIC(4)][LENGTH(4 little endian)][DATA(len bytes)]
// On n'utilise que les 3 canaux B,G,R par pixel (alpha laissé intact).
bool EmbedLSB(BYTE* pixels, size_t size, const std::string& msg) {
    if (!pixels) return false;

    // Buffer total à écrire
    const uint32_t len = (uint32_t)msg.size();
    const size_t total = 4 + 4 + (size_t)len; // magic + length + data

    // Capacité : nombre de pixels * 3 bits (B,G,R)
    const size_t pixelsCount = size / 4;
    const size_t capacityBits = pixelsCount * 3;
    const size_t neededBits = total * 8ull;
    if (neededBits > capacityBits) return false; // pas assez de place

    // Index bit courant dans le flux d'écriture (0..capacityBits-1)
    size_t bitIndex = 0;

    // Lambda local pour écrire un octet bit par bit (LSB-first).
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

// ExtractLSB : lit la signature puis la longueur et enfin les données.
// Retourne la string (UTF-8) si successful, sinon string vide.
std::string ExtractLSB(const BYTE* pixels, size_t size) {
    std::string out;
    if (!pixels) return out;

    const size_t pixelsCount = size / 4;
    const size_t capacityBits = pixelsCount * 3;

    // Lambda pour lire un octet à partir du bitIndex courant (LSB-first).
    auto readByteAt = [&](size_t& bitIndex) -> BYTE {
        BYTE v = 0;
        for (int i = 0; i < 8; ++i) {
            if (bitIndex >= capacityBits) return 0; // sécurité : pas assez de bits
            size_t p = bitIndex / 3;
            int c = (int)(bitIndex % 3);
            const BYTE* px = pixels + p * 4;
            int bit = (c == 0) ? get_bit(px[0]) : (c == 1) ? get_bit(px[1]) : get_bit(px[2]);
            v |= (BYTE)(bit << i);
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
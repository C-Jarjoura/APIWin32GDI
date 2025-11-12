// ImageManager.cpp
#include "ImageManager.h"
#include <cstdio>
#include <cstdlib>


#pragma pack(push, 1)
struct BMPFILEHEADER_ {
    WORD  bfType;      // 'BM' = 0x4D42
    DWORD bfSize;
    WORD  bfReserved1;
    WORD  bfReserved2;
    DWORD bfOffBits;
};
#pragma pack(pop)

static inline size_t RowStride24(int width) {
    // chaque ligne 24bpp est alignée sur 4 octets
    size_t raw = (size_t)width * 3;
    return (raw + 3u) & ~3u;
}

bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info32, BYTE*& data32) {
    info32 = nullptr;
    data32 = nullptr;

    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"rb");

    if (!f) return false;

    BMPFILEHEADER_ bf{};
    if (fread(&bf, sizeof(bf), 1, f) != 1) { fclose(f); return false; }
    if (bf.bfType != 0x4D42) { fclose(f); return false; } // 'BM'

    BITMAPINFOHEADER bih{};
    if (fread(&bih, sizeof(bih), 1, f) != 1) { fclose(f); return false; }

    if (bih.biCompression != BI_RGB) { fclose(f); return false; } // sans compression
    if (!(bih.biBitCount == 24 || bih.biBitCount == 32)) { fclose(f); return false; }

    const int width = (int)bih.biWidth;
    const int height = (int)bih.biHeight; // peut être <0 (top-down)
    const int absH = height < 0 ? -height : height;

    // Aller au début des pixels
    fseek(f, (long)bf.bfOffBits, SEEK_SET);

    // Lecture brute source
    BYTE* src = nullptr;
    size_t srcSize = 0;

    if (bih.biBitCount == 24) {
        size_t stride = RowStride24(width);
        srcSize = stride * absH;
        src = (BYTE*)malloc(srcSize);
        if (!src) { fclose(f); return false; }
        if (fread(src, 1, srcSize, f) != srcSize) { free(src); fclose(f); return false; }
    }
    else { // 32 bpp
        size_t stride = (size_t)width * 4;
        srcSize = stride * absH;
        src = (BYTE*)malloc(srcSize);
        if (!src) { fclose(f); return false; }
        if (fread(src, 1, srcSize, f) != srcSize) { free(src); fclose(f); return false; }
    }
    fclose(f);

    // Créer un DIB 32 bits BGRA (bottom-up)
    info32 = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    if (!info32) { free(src); return false; }

    info32->bmiHeader = {};
    info32->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info32->bmiHeader.biWidth = width;
    info32->bmiHeader.biHeight = absH;          // bottom-up
    info32->bmiHeader.biPlanes = 1;
    info32->bmiHeader.biBitCount = 32;
    info32->bmiHeader.biCompression = BI_RGB;
    info32->bmiHeader.biSizeImage = (DWORD)((size_t)width * 4 * absH);

    data32 = new BYTE[info32->bmiHeader.biSizeImage];
    if (!data32) { free(info32); info32 = nullptr; free(src); return false; }

    // Conversion vers BGRA 32
    if (bih.biBitCount == 24) {
        size_t srcStride = RowStride24(width);
        for (int y = 0; y < absH; ++y) {
            // BMP bottom-up si height>0 ; si height<0 (top-down), inverser la source
            const BYTE* srcLine = (height > 0)
                ? src + (size_t)(absH - 1 - y) * srcStride
                : src + (size_t)y * srcStride;

            BYTE* dstLine = data32 + (size_t)y * width * 4;
            for (int x = 0; x < width; ++x) {
                BYTE b = srcLine[x * 3 + 0];
                BYTE g = srcLine[x * 3 + 1];
                BYTE r = srcLine[x * 3 + 2];
                dstLine[x * 4 + 0] = b;
                dstLine[x * 4 + 1] = g;
                dstLine[x * 4 + 2] = r;
                dstLine[x * 4 + 3] = 0xFF; // alpha à 255
            }
        }
    }
    else { // 32 bpp -> réordonner lignes si besoin, assurer alpha
        size_t stride32 = (size_t)width * 4;
        for (int y = 0; y < absH; ++y) {
            const BYTE* srcLine = (height > 0)
                ? src + (size_t)(absH - 1 - y) * stride32
                : src + (size_t)y * stride32;
            BYTE* dstLine = data32 + (size_t)y * stride32;
            memcpy(dstLine, srcLine, stride32);
            // s’assurer alpha = 255 pour affichage GDI correct
            for (int x = 0; x < width; ++x) dstLine[x * 4 + 3] = 0xFF;
        }
    }

    free(src);
    return true;
}

bool SaveBMP(const wchar_t* filename, const BITMAPINFO* info32, const BYTE* data32) {
    if (!info32 || !data32) return false;
    const int width = info32->bmiHeader.biWidth;
    const int height = info32->bmiHeader.biHeight; // bottom-up (positif)
    const int absH = height < 0 ? -height : height;

    // On enregistre en 24 bpp (BGR) bottom-up
    size_t stride24 = RowStride24(width);
    size_t pixelDataSize = stride24 * absH;

    BMPFILEHEADER_ bf{};
    bf.bfType = 0x4D42;
    bf.bfOffBits = sizeof(BMPFILEHEADER_) + sizeof(BITMAPINFOHEADER);
    bf.bfSize = bf.bfOffBits + (DWORD)pixelDataSize;

    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = absH; // bottom-up
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = (DWORD)pixelDataSize;

    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"wb");

    if (!f) return false;

    bool ok = true;
    ok = ok && fwrite(&bf, sizeof(bf), 1, f) == 1;
    ok = ok && fwrite(&bih, sizeof(bih), 1, f) == 1;

    BYTE* line = (BYTE*)malloc(stride24);
    if (!line) { fclose(f); return false; }

    for (int y = 0; y < absH && ok; ++y) {
        const BYTE* srcLine = data32 + (size_t)(absH - 1 - y) * width * 4; // écrire bottom-up
        // convertir BGRA -> BGR + padding
        for (int x = 0; x < width; ++x) {
            line[x * 3 + 0] = srcLine[x * 4 + 0]; // B
            line[x * 3 + 1] = srcLine[x * 4 + 1]; // G
            line[x * 3 + 2] = srcLine[x * 4 + 2]; // R
        }
        // padding zéro
        for (size_t pad = (size_t)width * 3; pad < stride24; ++pad) line[pad] = 0;
        ok = ok && fwrite(line, 1, stride24, f) == stride24;
    }

    free(line);
    fclose(f);
    return ok;
}

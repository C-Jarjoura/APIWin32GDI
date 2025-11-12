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

bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info, BYTE*& data)
{
    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"rb");
    if (!f) return false;

    if (!f) return false;

    BITMAPFILEHEADER fileHeader;
    fread(&fileHeader, sizeof(fileHeader), 1, f);

    BITMAPINFOHEADER infoHeader;
    fread(&infoHeader, sizeof(infoHeader), 1, f);

    // Allocation du buffer image
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    memcpy(&info->bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));

    int imageSize = infoHeader.biSizeImage;
    if (imageSize == 0)
        imageSize = infoHeader.biWidth * abs(infoHeader.biHeight) * 3;

    data = new BYTE[imageSize];
    fread(data, 1, imageSize, f);
    fclose(f);

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

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "ImageManager.h"

// ============================================================
//  LoadBMP : charge BMP 24/32 bits et convertit en DIB 32 bits BGRA top-down
// ============================================================
bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info, BYTE*& data)
{
    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"rb");
    if (!f)
    {
        MessageBox(NULL, L"Impossible d'ouvrir le fichier BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    BITMAPFILEHEADER fileHeader;
    fread(&fileHeader, sizeof(fileHeader), 1, f);

    if (fileHeader.bfType != 0x4D42) // "BM"
    {
        fclose(f);
        MessageBox(NULL, L"Ce fichier n'est pas un BMP valide.", L"Erreur", MB_ICONERROR);
        return false;
    }

    BITMAPINFOHEADER infoHeader;
    fread(&infoHeader, sizeof(infoHeader), 1, f);

    if (infoHeader.biCompression != BI_RGB)
    {
        fclose(f);
        MessageBox(NULL, L"Seuls les BMP non compressés sont supportés.", L"Erreur", MB_ICONERROR);
        return false;
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bpp = infoHeader.biBitCount;

    int srcBytesPerPixel = (bpp == 32) ? 4 : 3;
    int srcStride = (width * srcBytesPerPixel + 3) & ~3;
    int srcSize = srcStride * height;

    // Lecture des pixels bruts
    BYTE* srcData = new BYTE[srcSize];

    fseek(f, fileHeader.bfOffBits, SEEK_SET);
    fread(srcData, 1, srcSize, f);
    fclose(f);

    // Création d’un DIB 32 bits TOP-DOWN
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    ZeroMemory(info, sizeof(BITMAPINFOHEADER));

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = -height;      // TOP-DOWN
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = width * 4 * height;
    info->bmiHeader.biXPelsPerMeter = infoHeader.biXPelsPerMeter;
    info->bmiHeader.biYPelsPerMeter = infoHeader.biYPelsPerMeter;

    data = new BYTE[info->bmiHeader.biSizeImage];

    // Conversion vers BGRA 32 bits + top-down
    for (int y = 0; y < height; ++y)
    {
        int srcY = (infoHeader.biHeight > 0) ? (height - 1 - y) : y; // inversion si bottom-up

        BYTE* srcRow = srcData + srcY * srcStride;
        BYTE* dstRow = data + y * width * 4;

        if (bpp == 32)
        {
            // Copie directe BGRA
            memcpy(dstRow, srcRow, width * 4);
        }
        else
        {
            // Conversion 24 -> 32 BGRA
            for (int x = 0; x < width; ++x)
            {
                BYTE b = srcRow[x * 3 + 0];
                BYTE g = srcRow[x * 3 + 1];
                BYTE r = srcRow[x * 3 + 2];

                dstRow[x * 4 + 0] = b;
                dstRow[x * 4 + 1] = g;
                dstRow[x * 4 + 2] = r;
                dstRow[x * 4 + 3] = 0xFF; // opaque
            }
        }
    }

    delete[] srcData;

    return true;
}

// ============================================================
//  SaveBMP : sauvegarde un DIB BGRA top-down au format BMP
// ============================================================
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info)
{
    if (!data || !info) return false;

    int width = info->bmiHeader.biWidth;
    int height = abs(info->bmiHeader.biHeight);
    int bpp = info->bmiHeader.biBitCount;

    bool is32bpp = (bpp == 32);
    int bytesPerPixel = is32bpp ? 4 : 3;

    int stride = (width * bytesPerPixel + 3) & ~3;
    DWORD pixelDataSize = stride * height;

    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER hdr = info->bmiHeader;
    hdr.biCompression = BI_RGB;
    hdr.biHeight = -height; // garder top-down

    DWORD fileSize = sizeof(fileHeader) + sizeof(hdr) + pixelDataSize;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(hdr);
    fileHeader.bfSize = fileSize;

    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"wb");
    if (!f)
    {
        MessageBox(NULL, L"Impossible d'écrire le BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    fwrite(&fileHeader, sizeof(fileHeader), 1, f);
    fwrite(&hdr, sizeof(hdr), 1, f);

    fwrite(data, 1, pixelDataSize, f);

    fclose(f);
    return true;
}

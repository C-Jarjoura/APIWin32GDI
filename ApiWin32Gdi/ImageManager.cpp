#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "ImageManager.h"

// ============================================================
//  Fonction : LoadBMP
//  Objectif : Charger un BMP 24 ou 32 bits, le convertir en top-down
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
    int bytesPerPixel = (bpp == 32) ? 4 : 3;
    int stride = ((width * bytesPerPixel + 3) & ~3);
    int dataSize = stride * height;

    // Allocation
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    memcpy(&info->bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));
    data = new BYTE[dataSize];

    fseek(f, fileHeader.bfOffBits, SEEK_SET);
    fread(data, 1, dataSize, f);
    fclose(f);

    // === Conversion vers top-down si nécessaire ===
    if (infoHeader.biHeight > 0)
    {
        BYTE* flipped = new BYTE[dataSize];
        for (int y = 0; y < height; ++y)
        {
            memcpy(flipped + y * stride, data + (height - 1 - y) * stride, stride);
        }
        delete[] data;
        data = flipped;
        info->bmiHeader.biHeight = -height; // top-down
    }
    else
    {
        info->bmiHeader.biHeight = -height; // déjà top-down
    }

    return true;
}

// ============================================================
//  Fonction : SaveBMP
//  Objectif : Sauvegarder un BMP dans le même sens que la mémoire (top-down)
// ============================================================
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info)
{
    if (!data || !info) return false;

    int width = info->bmiHeader.biWidth;
    int height = abs(info->bmiHeader.biHeight);
    int bpp = info->bmiHeader.biBitCount;
    bool is32bpp = (bpp == 32);
    int bytesPerPixel = is32bpp ? 4 : 3;
    int stride = ((width * bytesPerPixel + 3) & ~3);
    DWORD pixelDataSize = stride * height;

    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = info->bmiHeader;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biHeight = -height; // on garde top-down

    DWORD fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pixelDataSize;
    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileSize;

    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"wb");
    if (!f)
    {
        MessageBox(NULL, L"Impossible de créer le fichier BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    fwrite(&fileHeader, sizeof(fileHeader), 1, f);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f);
    fwrite(data, 1, pixelDataSize, f);
    fclose(f);

    return true;
}

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include "ImageManager.h"

// ============================================================
//  Fonction : LoadBMP
//  Objectif : Charger une image BMP 24 ou 32 bits en mémoire
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

    // Lecture uniquement des BMP non compressés (BI_RGB)
    if (infoHeader.biCompression != BI_RGB)
    {
        fclose(f);
        MessageBox(NULL, L"Seuls les BMP non compressés sont supportés.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Allocation de la structure BITMAPINFO
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    memcpy(&info->bmiHeader, &infoHeader, sizeof(BITMAPINFOHEADER));

    // Taille de l'image en octets
    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bpp = infoHeader.biBitCount;
    int bytesPerPixel = (bpp == 32) ? 4 : 3;

    int dataSize = width * height * bytesPerPixel;
    data = new BYTE[dataSize];

    // Aller à la position du bitmap
    fseek(f, fileHeader.bfOffBits, SEEK_SET);
    fread(data, 1, dataSize, f);
    fclose(f);

    // BMP bottom-up → inversion de la hauteur
    if (info->bmiHeader.biHeight > 0)
        info->bmiHeader.biHeight = -info->bmiHeader.biHeight;

    return true;
}

// ============================================================
//  Fonction : SaveBMP
//  Objectif : Sauvegarder une image BMP 24 bits conforme
// ============================================================
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info)
{
    if (!data || !info) return false;

    int width = info->bmiHeader.biWidth;
    int height = abs(info->bmiHeader.biHeight);
    int bpp = info->bmiHeader.biBitCount;
    bool is32bpp = (bpp == 32);

    // Vérification de la compatibilité
    if (bpp != 24 && bpp != 32)
    {
        MessageBox(NULL, L"Format non supporté : seules les images 24 ou 32 bits sont autorisées.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // === Étape 1 : Préparer les headers ===
    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER infoHeader = info->bmiHeader;

    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = 0;

    int stride = ((width * 3 + 3) & ~3); // padding à 4 octets
    DWORD pixelDataSize = stride * height;
    DWORD fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pixelDataSize;

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileSize;

    // === Étape 2 : Ouverture du fichier ===
    FILE* f = nullptr;
    _wfopen_s(&f, filename, L"wb");
    if (!f)
    {
        MessageBox(NULL, L"Impossible de créer le fichier BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    fwrite(&fileHeader, sizeof(fileHeader), 1, f);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f);

    // === Étape 3 : Écriture des pixels (BGR + padding) ===
    BYTE* line = new BYTE[stride];
    memset(line, 0, stride);

    for (int y = 0; y < height; ++y)
    {
        const BYTE* srcLine = data + (size_t)y * width * (is32bpp ? 4 : 3);

        for (int x = 0; x < width; ++x)
        {
            int srcOffset = x * (is32bpp ? 4 : 3);
            line[x * 3 + 0] = srcLine[srcOffset + 0]; // B
            line[x * 3 + 1] = srcLine[srcOffset + 1]; // G
            line[x * 3 + 2] = srcLine[srcOffset + 2]; // R
        }

        fwrite(line, 1, stride, f);
    }

    delete[] line;
    fclose(f);
    return true;
}

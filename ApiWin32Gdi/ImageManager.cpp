#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "ImageManager.h"

// ============================================================
//  LoadBMP : charge BMP 24/32 bits et convertit en DIB 32 bits BGRA top-down
// Notes importantes:
// - Supporte uniquement BMP non compressés (biCompression == BI_RGB).
// - Retourne info (BITMAPINFO*) alloué via malloc(sizeof(BITMAPINFOHEADER))
//   et data (BYTE*) alloué via new BYTE[..].
// - L'image résultante est top-down (biHeight négatif) et 32bpp BGRA.
// ============================================================
bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info, BYTE*& data)
{
    HANDLE hFile = CreateFileW(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"Impossible d'ouvrir le fichier BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    DWORD bytesRead = 0;
    BITMAPFILEHEADER fileHeader;
    // Vérifier la réussite de ReadFile et le nombre d'octets lus
    if (!ReadFile(hFile, &fileHeader, sizeof(fileHeader), &bytesRead, NULL) || bytesRead != sizeof(fileHeader))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de la lecture de l'en-tête BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Vérifier la signature 'BM'
    if (fileHeader.bfType != 0x4D42)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Ce fichier n'est pas un BMP valide.", L"Erreur", MB_ICONERROR);
        return false;
    }

    BITMAPINFOHEADER infoHeader;
    // Lire l'en-tête BITMAPINFOHEADER en vérifiant la lecture
    if (!ReadFile(hFile, &infoHeader, sizeof(infoHeader), &bytesRead, NULL) || bytesRead != sizeof(infoHeader))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de la lecture de l'en-tête BITMAPINFO.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Seuls les BMP non compressés sont supportés (BI_RGB)
    if (infoHeader.biCompression != BI_RGB)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Seuls les BMP non compressés sont supportés.", L"Erreur", MB_ICONERROR);
        return false;
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bpp = infoHeader.biBitCount;

    // Défense basique : vérifier valeurs plausibles pour éviter overflows
    if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Format BMP non supporté (dimensions ou profondeur invalides).", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Calcul du stride source (aligné sur 4 octets)
    int srcBytesPerPixel = (bpp == 32 ? 4 : 3);
    const long long tmpStride = (long long)width * srcBytesPerPixel;
    if (tmpStride > INT_MAX) {
        CloseHandle(hFile);
        MessageBox(NULL, L"Image trop large.", L"Erreur", MB_ICONERROR);
        return false;
    }
    int srcStride = (int)((tmpStride + 3) & ~3);
    long long tmpSrcSize = (long long)srcStride * height;
    if (tmpSrcSize > INT_MAX) {
        CloseHandle(hFile);
        MessageBox(NULL, L"Image trop large.", L"Erreur", MB_ICONERROR);
        return false;
    }
    int srcSize = (int)tmpSrcSize;

    BYTE* srcData = nullptr;
    try {
        srcData = new BYTE[srcSize];
    }
    catch (...) {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur d'allocation mémoire pour l'image source.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Positionner le curseur sur les pixels
    if (SetFilePointer(hFile, fileHeader.bfOffBits, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        delete[] srcData;
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors du positionnement du pointeur fichier.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Lire les données pixels et vérifier le succès et la taille lue
    if (!ReadFile(hFile, srcData, srcSize, &bytesRead, NULL) || (int)bytesRead != srcSize)
    {
        delete[] srcData;
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de la lecture des données de l'image BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    CloseHandle(hFile);

    // Création DIB 32 bits BGRA top-down
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    if (!info) {
        delete[] srcData;
        MessageBox(NULL, L"Erreur d'allocation mémoire.", L"Erreur", MB_ICONERROR);
        return false;
    }
    ZeroMemory(info, sizeof(BITMAPINFOHEADER));

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    // Mettre height négatif pour indiquer top-down (ligne 0 = top)
    info->bmiHeader.biHeight = -height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    // biSizeImage = width * 4 * height ; vérifier overflow avant de l'assigner
    long long tmpSizeImage = (long long)width * 4ll * height;
    if (tmpSizeImage > INT_MAX) {
        free(info);
        delete[] srcData;
        MessageBox(NULL, L"Image trop grande.", L"Erreur", MB_ICONERROR);
        return false;
    }
    info->bmiHeader.biSizeImage = (DWORD)tmpSizeImage;

    try {
        data = new BYTE[info->bmiHeader.biSizeImage];
    }
    catch (...) {
        free(info);
        delete[] srcData;
        MessageBox(NULL, L"Erreur d'allocation mémoire pour les pixels convertis.", L"Erreur", MB_ICONERROR);
        return false;
    }

    // Pré-calculs pour vérifications de bornes
    const size_t srcDataSizeU = (size_t)srcSize;
    const size_t dstDataSizeU = (size_t)info->bmiHeader.biSizeImage;
    const size_t expectedDstRowBytes = (size_t)width * 4u;
    const size_t expectedSrcRowBytes = (size_t)width * (size_t)srcBytesPerPixel;

    // Vérification rapide avant la boucle : s'assurer que les tampons sont suffisamment grands
    if (expectedDstRowBytes == 0 || expectedSrcRowBytes == 0) {
        free(info);
        delete[] srcData;
        delete[] data;
        MessageBox(NULL, L"Dimensions invalides.", L"Erreur", MB_ICONERROR);
        return false;
    }
    // S'assurer que le buffer de destination total est cohérent
    if (dstDataSizeU < expectedDstRowBytes * (size_t)height) {
        free(info);
        delete[] srcData;
        delete[] data;
        MessageBox(NULL, L"Buffer de destination trop petit.", L"Erreur", MB_ICONERROR);
        return false;
    }
    // S'assurer que le buffer source total est cohérent
    if (srcDataSizeU < expectedSrcRowBytes * (size_t)height) {
        if (srcDataSizeU < (size_t)srcStride * (size_t)height) {
            free(info);
            delete[] srcData;
            delete[] data;
            MessageBox(NULL, L"Buffer source trop petit.", L"Erreur", MB_ICONERROR);
            return false;
        }
    }

    for (int y = 0; y < height; y++)
    {
        // Si infoHeader.biHeight > 0, source est bottom-up : on inverse l'ordre des lignes.
        int srcY = (infoHeader.biHeight > 0) ? (height - 1 - y) : y;

        // Calcul d'offsets en taille non signée pour les vérifications
        const size_t srcRowOffset = (size_t)srcY * (size_t)srcStride;
        const size_t dstRowOffset = (size_t)y * expectedDstRowBytes;

        if (srcRowOffset + expectedSrcRowBytes > srcDataSizeU || dstRowOffset + expectedDstRowBytes > dstDataSizeU) {
            // Nettoyage et message d'erreur clair indiquant l'image problématique
            free(info);
            delete[] srcData;
            delete[] data;
            MessageBox(NULL, L"Données image invalides ou corruptes (dépassement de ligne détecté).", L"Erreur", MB_ICONERROR);
            return false;
        }

        BYTE* srcRow = srcData + srcRowOffset;
        BYTE* dstRow = data + dstRowOffset;

        if (bpp == 32)
        {
            // Si la source est déjà 32bpp, on copie uniquement les octets utiles (width*4)
            memcpy(dstRow, srcRow, expectedDstRowBytes);
        }
        else
        {
            for (int x = 0; x < width; x++)
            {
                // Calcul des offsets internes sûrs
                size_t srcPixelOffset = (size_t)x * 3u;
                size_t dstPixelOffset = (size_t)x * 4u;

                dstRow[dstPixelOffset + 0] = srcRow[srcPixelOffset + 0]; // B
                dstRow[dstPixelOffset + 1] = srcRow[srcPixelOffset + 1]; // G
                dstRow[dstPixelOffset + 2] = srcRow[srcPixelOffset + 2]; // R
                dstRow[dstPixelOffset + 3] = 0xFF;                        // A
            }
        }
    }

    delete[] srcData;
    return true;
}

// ============================================================
//  SaveBMP : sauvegarde un DIB BGRA top-down en BMP non compressé.
//  Ajout : vérification des retours de WriteFile pour détecter erreurs d'écriture.
// ============================================================
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info)
{
    if (!data || !info)
        return false;

    HANDLE hFile = CreateFileW(
        filename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"Impossible d'écrire le fichier BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }

    DWORD bytesWritten = 0;

    int width = info->bmiHeader.biWidth;
    int height = abs(info->bmiHeader.biHeight);

    // Stride pour 32bpp (déjà multiple de 4)
    int stride = (width * 4 + 3) & ~3;
    long long tmpPixelDataSize = (long long)stride * height;
    if (tmpPixelDataSize > INT_MAX) {
        CloseHandle(hFile);
        MessageBox(NULL, L"Image trop grande pour l'écriture.", L"Erreur", MB_ICONERROR);
        return false;
    }
    DWORD pixelDataSize = (DWORD)tmpPixelDataSize;

    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER hdr = info->bmiHeader;

    // Conserver le top-down en écrivant biHeight négatif
    hdr.biHeight = -height;
    hdr.biCompression = BI_RGB;

    fileHeader.bfType = 0x4D42; // 'BM'
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(hdr);
    fileHeader.bfSize = fileHeader.bfOffBits + pixelDataSize;

    // Ecrire fileHeader
    if (!WriteFile(hFile, &fileHeader, sizeof(fileHeader), &bytesWritten, NULL) || bytesWritten != sizeof(fileHeader))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de l'écriture de l'en-tête BMP.", L"Erreur", MB_ICONERROR);
        return false;
    }
    // Ecrire hdr
    if (!WriteFile(hFile, &hdr, sizeof(hdr), &bytesWritten, NULL) || bytesWritten != sizeof(hdr))
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de l'écriture de l'en-tête BITMAPINFO.", L"Erreur", MB_ICONERROR);
        return false;
    }
    // Ecrire les pixels
    if (!WriteFile(hFile, data, pixelDataSize, &bytesWritten, NULL) || bytesWritten != pixelDataSize)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Erreur lors de l'écriture des données pixels.", L"Erreur", MB_ICONERROR);
        return false;
    }

    CloseHandle(hFile);
    return true;
}
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "ImageManager.h"

// ============================================================
//  LoadBMP : charge BMP 24/32 bits et convertit en DIB 32 bits BGRA top-down
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
    ReadFile(hFile, &fileHeader, sizeof(fileHeader), &bytesRead, NULL);

    if (fileHeader.bfType != 0x4D42)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Ce fichier n'est pas un BMP valide.", L"Erreur", MB_ICONERROR);
        return false;
    }

    BITMAPINFOHEADER infoHeader;
    ReadFile(hFile, &infoHeader, sizeof(infoHeader), &bytesRead, NULL);

    if (infoHeader.biCompression != BI_RGB)
    {
        CloseHandle(hFile);
        MessageBox(NULL, L"Seuls les BMP non compressés sont supportés.", L"Erreur", MB_ICONERROR);
        return false;
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bpp = infoHeader.biBitCount;

    int srcBytesPerPixel = (bpp == 32 ? 4 : 3);
    int srcStride = (width * srcBytesPerPixel + 3) & ~3;
    int srcSize = srcStride * height;

    BYTE* srcData = new BYTE[srcSize];

    // Positionner le curseur sur les pixels
    SetFilePointer(hFile, fileHeader.bfOffBits, NULL, FILE_BEGIN);

    ReadFile(hFile, srcData, srcSize, &bytesRead, NULL);
    CloseHandle(hFile);

    // Création DIB 32 bits BGRA top-down
    info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER));
    ZeroMemory(info, sizeof(BITMAPINFOHEADER));

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = -height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = width * 4 * height;

    data = new BYTE[info->bmiHeader.biSizeImage];

    // Conversion BGR -> BGRA + inversion éventuelle
    for (int y = 0; y < height; y++)
    {
        int srcY = (infoHeader.biHeight > 0) ? (height - 1 - y) : y;
        BYTE* srcRow = srcData + srcY * srcStride;
        BYTE* dstRow = data + y * width * 4;

        if (bpp == 32)
        {
            memcpy(dstRow, srcRow, width * 4);
        }
        else
        {
            for (int x = 0; x < width; x++)
            {
                dstRow[x * 4 + 0] = srcRow[x * 3 + 0];
                dstRow[x * 4 + 1] = srcRow[x * 3 + 1];
                dstRow[x * 4 + 2] = srcRow[x * 3 + 2];
                dstRow[x * 4 + 3] = 0xFF;
            }
        }
    }

    delete[] srcData;
    return true;
}

// ============================================================
//  SaveBMP : sauvegarde un DIB BGRA top-down en BMP
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

    int stride = (width * 4 + 3) & ~3;
    DWORD pixelDataSize = stride * height;

    BITMAPFILEHEADER fileHeader = {};
    BITMAPINFOHEADER hdr = info->bmiHeader;

    hdr.biHeight = -height;
    hdr.biCompression = BI_RGB;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(hdr);
    fileHeader.bfSize = fileHeader.bfOffBits + pixelDataSize;

    WriteFile(hFile, &fileHeader, sizeof(fileHeader), &bytesWritten, NULL);
    WriteFile(hFile, &hdr, sizeof(hdr), &bytesWritten, NULL);
    WriteFile(hFile, data, pixelDataSize, &bytesWritten, NULL);

    CloseHandle(hFile);
    return true;
}

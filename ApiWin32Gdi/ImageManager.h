#pragma once
#include <windows.h>

// Fonctions pour charger et sauvegarder des BMP non compressés.
// - LoadBMP : charge un fichier BMP 24/32 bits et convertit en DIB 32 bits BGRA top-down.
//            Alloue 'info' (via malloc) et 'data' (via new[]).
// - SaveBMP : sauvegarde un DIB BGRA top-down en fichier BMP non compressé.
//
// Remarque : l'appelant est responsable de free(g_pInfo) et delete[] g_pPixels.
bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info, BYTE*& data);
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info);
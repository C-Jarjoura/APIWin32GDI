// ImageManager.h
#pragma once
#include <windows.h>

bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info32, BYTE*& data32);
bool SaveBMP(const wchar_t* filename, const BITMAPINFO* info32, const BYTE* data32);

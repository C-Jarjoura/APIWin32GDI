// ImageManager.h
#pragma once
#include <windows.h>

bool LoadBMP(const wchar_t* filename, BITMAPINFO*& info, BYTE*& data);
bool SaveBMP(const wchar_t* filename, BYTE* data, BITMAPINFO* info);
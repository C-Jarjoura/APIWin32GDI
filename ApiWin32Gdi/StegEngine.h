// StegEngine.h
#pragma once
#include <windows.h>
#include <string>

bool EmbedLSB(BYTE* pixelsBGRA, size_t byteSize, const std::string& messageUtf8);
std::string ExtractLSB(const BYTE* pixelsBGRA, size_t byteSize);

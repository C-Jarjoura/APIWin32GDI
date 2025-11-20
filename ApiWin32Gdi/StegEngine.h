#pragma once
#include <windows.h>
#include <string>

// API minimaliste de la stéganographie LSB utilisée par l'application.
// - EmbedLSB : écrit MAGIC + length + data dans les LSB des canaux B,G,R des pixels (format BGRA 32bpp).
// - ExtractLSB : lit le paquet si MAGIC est présent et retourne le message (UTF-8) ou string vide.
bool EmbedLSB(BYTE* pixelsBGRA, size_t byteSize, const std::string& messageUtf8);
std::string ExtractLSB(const BYTE* pixelsBGRA, size_t byteSize);
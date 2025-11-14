#pragma once
#include <windows.h>
#include "StegEngine.h"
#include "ImageManager.h"

void DialogEmbedMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info);
void DialogExtractMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info);
void ShowAboutDialog(HWND hwnd);  

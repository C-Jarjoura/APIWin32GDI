#pragma once
#include <windows.h>

// Prototype : affiche une image DIB 32bpp BGRA via GDI dans un HDC.
// - hdc : contexte de device de la fenêtre (BeginPaint/EndPaint).
// - info32 : BITMAPINFO décrivant le DIB (doit contenir un BITMAPINFOHEADER).
// - data32 : pointeur sur les pixels au format BGRA 32 bits.
void RenderImage(HDC hdc, BITMAPINFO* info32, BYTE* data32);
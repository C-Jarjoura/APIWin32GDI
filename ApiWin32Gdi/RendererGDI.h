#pragma once
#include <windows.h>

// Prend les coordonnées (centerX, centerY) et le facteur de zoom
void RenderImage(
    HDC hdc,
    BITMAPINFO* info32,
    BYTE* data32,
    double zoom = 1.0,
    int zoomCenterX = -1,
    int zoomCenterY = -1
);
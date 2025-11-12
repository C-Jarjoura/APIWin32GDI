#include <windows.h>
#include "RendererGDI.h"

// ============================================================
//  Fonction : RenderImage
//  Objectif : Afficher une image BMP (24/32 bits) à l’écran
// ============================================================
void RenderImage(HDC hdc, BITMAPINFO* info, BYTE* pixels)
{
    if (!hdc || !info || !pixels)
        return;

    int width = info->bmiHeader.biWidth;
    int height = abs(info->bmiHeader.biHeight);
    int bpp = info->bmiHeader.biBitCount;

    // Récupère la taille de la zone cliente
    RECT rc;
    GetClientRect(WindowFromDC(hdc), &rc);
    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;

    // Centre l’image dans la fenêtre
    int x = (clientWidth - width) / 2;
    int y = (clientHeight - height) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    // Affichage avec GDI
    // BitBlt : copie directe (rapide, mais taille fixe)
    // StretchDIBits : permet redimensionnement ou affichage partiel
    StretchDIBits(
        hdc,
        x, y, width, height,         // destination
        0, 0, width, height,         // source
        pixels,
        info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
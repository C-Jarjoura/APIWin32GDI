#include <windows.h>
#include "RendererGDI.h"

// ============================================================
// Fonction utilitaire : ajuste une image ・la fenêtre
// tout en conservant son ratio largeur/hauteur
// ============================================================
static void FitRectKeepAspect(int imgW, int imgH, const RECT& dst, RECT& out)
{
    int dstW = dst.right - dst.left;
    int dstH = dst.bottom - dst.top;

    if (imgW <= 0 || imgH <= 0 || dstW <= 0 || dstH <= 0)
    {
        out = dst;
        return;
    }

    // Calcul du facteur d'échelle (le plus petit)
    double sx = (double)dstW / imgW;
    double sy = (double)dstH / imgH;
    double s = (sx < sy) ? sx : sy;

    int w = (int)(imgW * s + 0.5);
    int h = (int)(imgH * s + 0.5);
    int x = dst.left + (dstW - w) / 2;
    int y = dst.top + (dstH - h) / 2;

    out.left = x;
    out.top = y;
    out.right = x + w;
    out.bottom = y + h;
}

// ============================================================
// Fonction : RenderImage
// Objectif : afficher un BMP 24/32 bits dans la fenêtre,
//             ajust・・la taille tout en gardant le ratio
// ============================================================
void RenderImage(HDC hdc, BITMAPINFO* info32, BYTE* data32)
{
    if (!hdc || !info32 || !data32)
        return;

    // Taille de l'image source
    int imgW = info32->bmiHeader.biWidth;
    int imgH = abs(info32->bmiHeader.biHeight);

    // Get full client rect from the HDC's window (safer than GetClipBox
    // which can return a partial region and leave previous content visible)
    RECT client;
    HWND hwnd = WindowFromDC(hdc);
    if (hwnd)
        GetClientRect(hwnd, &client);
    else
        GetClipBox(hdc, &client);

    // Clear the background so there is no ghost/duplicate image left.
    // Use the window background color (COLOR_WINDOW+1) to match the original.
    HBRUSH hbr = (HBRUSH)(COLOR_WINDOW + 1);
    // FillRect expects an HBRUSH handle; convert COLOR_* to brush with GetSysColorBrush.
    FillRect(hdc, &client, GetSysColorBrush(COLOR_WINDOW));

    // Calcul du rectangle de destination ajust・
    RECT dstRect;
    FitRectKeepAspect(imgW, imgH, client, dstRect);

    // Affichage de l'image avec GDI
    StretchDIBits(
        hdc,
        dstRect.left, dstRect.top,
        dstRect.right - dstRect.left,
        dstRect.bottom - dstRect.top,
        0, 0,
        imgW, imgH,
        data32,
        info32,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// les fichiers enregistrer gardent leurs orientation mais les fichier telecharger jamais passer par l'api sont invers
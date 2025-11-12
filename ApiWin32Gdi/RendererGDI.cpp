#include <windows.h>
#include "RendererGDI.h"

// ============================================================
// Fonction utilitaire : ajuste une image à la fenêtre
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

    // Calcul du facteur d’échelle (le plus petit)
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
//             ajusté à la taille tout en gardant le ratio
// ============================================================
void RenderImage(HDC hdc, BITMAPINFO* info32, BYTE* data32)
{
    if (!hdc || !info32 || !data32)
        return;

    // Taille de l’image source
    int imgW = info32->bmiHeader.biWidth;
    int imgH = abs(info32->bmiHeader.biHeight);

    // Zone de dessin actuelle
    RECT client;
    GetClipBox(hdc, &client);

    // Calcul du rectangle de destination ajusté
    RECT dstRect;
    FitRectKeepAspect(imgW, imgH, client, dstRect);

    // Affichage de l’image avec GDI
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

// les fichiers enregistrer gardent leurs orientation mais les fichier telecharger jamais passer par l'api sont inversé
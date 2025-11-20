#include <windows.h>
#include "RendererGDI.h"

// ============================================================
// Fonction utilitaire : ajuste une image à la fenêtre
// tout en conservant son ratio largeur/hauteur.
// Entrées : imgW/imgH = taille source,
//           dst = rectangle de destination (client area),
// Sortie : out = rectangle centré et dimensionné pour conserver le ratio.
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

    // Calcul du facteur d'échelle (prendre le plus petit pour que l'image tienne)
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
//            ajuster la taille tout en gardant le ratio.
// Hypothèses : info32 décrit un DIB 32 bits (biBitCount=32) top-down (biHeight négatif),
//              data32 pointe sur les pixels en format BGRA (B,G,R,A).
// ============================================================
void RenderImage(HDC hdc, BITMAPINFO* info32, BYTE* data32)
{
    if (!hdc || !info32 || !data32)
        return;

    // Taille de l'image source
    int imgW = info32->bmiHeader.biWidth;
    int imgH = abs(info32->bmiHeader.biHeight);

    RECT client;
    HWND hwnd = WindowFromDC(hdc);
    if (hwnd)
        GetClientRect(hwnd, &client);
    else
        GetClipBox(hdc, &client);

    // Effacer le fond pour éviter les ghost/duplications résiduelles.
    // On utilise la couleur de fond de fenêtre pour que l'aspect soit cohérent.
    HBRUSH hbr = (HBRUSH)(COLOR_WINDOW + 1);
    FillRect(hdc, &client, GetSysColorBrush(COLOR_WINDOW));

    // Calcul du rectangle de destination qui conserve l'aspect.
    RECT dstRect;
    FitRectKeepAspect(imgW, imgH, client, dstRect);

    // Affichage de l'image avec StretchDIBits. StretchDIBits lit les pixels
    // fournis via data32 et interprète info32->bmiHeader pour le format.
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

// Note : commentaire non finalisé dans le dépôt original à propos d'orientation.
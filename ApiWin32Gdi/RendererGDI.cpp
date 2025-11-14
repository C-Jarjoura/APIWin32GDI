#include <windows.h>
#include "RendererGDI.h"

// ============================================================
// Ajuste une image à la fenêtre en conservant le ratio
// ============================================================
static void FitRectKeepAspect(int imgW, int imgH, const RECT& dst, RECT& out)
{
    int dstW = dst.right - dst.left;
    int dstH = dst.bottom - dst.top;

    if (imgW <= 0 || imgH <= 0 || dstW <= 0 || dstH <= 0) {
        out = dst;
        return;
    }

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
// Affiche un BMP 24/32bpp dans la fenêtre, ajusté et zoomé
// Zoom centré possible (zoomCenterX/Y en pixels fenêtre)
// ============================================================
void RenderImage(
    HDC hdc,
    BITMAPINFO* info32,
    BYTE* data32,
    double zoom,
    int zoomCenterX,
    int zoomCenterY)
{
    if (!hdc || !info32 || !data32)
        return;

    int imgW = info32->bmiHeader.biWidth;
    int imgH = abs(info32->bmiHeader.biHeight);

    RECT client;
    HWND hwnd = WindowFromDC(hdc);
    if (hwnd)
        GetClientRect(hwnd, &client);
    else
        GetClipBox(hdc, &client);

    // Efface le fond
    FillRect(hdc, &client, GetSysColorBrush(COLOR_WINDOW));

    // Rectangle destination "fit" sans zoom
    RECT dstRect;
    FitRectKeepAspect(imgW, imgH, client, dstRect);

    int baseW = dstRect.right - dstRect.left;
    int baseH = dstRect.bottom - dstRect.top;

    // Par défaut (pas de zoom center ou zoom 1.0) -> centré
    int wZoom = (int)(baseW * zoom);
    int hZoom = (int)(baseH * zoom);
    int xZoom, yZoom;

    if (zoomCenterX < 0 || zoomCenterY < 0 || zoom == 1.0) {
        // Centre normal dans client
        xZoom = (client.left + client.right - wZoom) / 2;
        yZoom = (client.top + client.bottom - hZoom) / 2;
    }
    else {
        // Zoom autour du point cliqué
        double relX = (baseW ? ((double)(zoomCenterX - dstRect.left) / baseW) : 0.5);
        double relY = (baseH ? ((double)(zoomCenterY - dstRect.top) / baseH) : 0.5);

        // Calcule l'origine pour que le "focus" reste sous la souris
        xZoom = zoomCenterX - (int)(relX * wZoom);
        yZoom = zoomCenterY - (int)(relY * hZoom);

        // Optionnel : garde l'image dans la fenêtre
        if (xZoom > client.right - 10) xZoom = client.right - 10;
        if (yZoom > client.bottom - 10) yZoom = client.bottom - 10;
        if (xZoom + wZoom < client.left + 10) xZoom = client.left + 10 - wZoom;
        if (yZoom + hZoom < client.top + 10) yZoom = client.top + 10 - hZoom;
    }

    RECT zoomRect = { xZoom, yZoom, xZoom + wZoom, yZoom + hZoom };

    StretchDIBits(
        hdc,
        zoomRect.left, zoomRect.top,
        zoomRect.right - zoomRect.left,
        zoomRect.bottom - zoomRect.top,
        0, 0,
        imgW, imgH,
        data32,
        info32,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
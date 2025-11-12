// RendererGDI.cpp
#include "RendererGDI.h"

static void FitRectKeepAspect(int imgW, int imgH, const RECT& dst, RECT& out) {
    int dstW = dst.right - dst.left;
    int dstH = dst.bottom - dst.top;
    if (imgW <= 0 || imgH <= 0 || dstW <= 0 || dstH <= 0) { out = dst; return; }

    // scale to fit
    double sx = (double)dstW / imgW;
    double sy = (double)dstH / imgH;
    double s = (sx < sy) ? sx : sy;

    int w = (int)(imgW * s + 0.5);
    int h = (int)(imgH * s + 0.5);
    int x = dst.left + (dstW - w) / 2;
    int y = dst.top + (dstH - h) / 2;
    out = { x, y, x + w, y + h };
}

void RenderImage(HDC hdc, BITMAPINFO* info32, BYTE* data32) {
    if (!hdc || !info32 || !data32) return;

    RECT client;
    GetClipBox(hdc, &client); // zone à peindre

    RECT dstRect;
    FitRectKeepAspect(info32->bmiHeader.biWidth, abs(info32->bmiHeader.biHeight), client, dstRect);

    StretchDIBits(
        hdc,
        dstRect.left, dstRect.top,
        dstRect.right - dstRect.left,
        dstRect.bottom - dstRect.top,
        0, 0,
        info32->bmiHeader.biWidth,
        abs(info32->bmiHeader.biHeight),
        data32,
        info32,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

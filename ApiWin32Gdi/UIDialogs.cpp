#include "UIDialogs.h"
#include <commdlg.h>
#include <string>

#include "resource.h"

// conversion Wide -> UTF-8
static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], needed, nullptr, nullptr);


    return out;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], needed);

    return out;
}

static INT_PTR CALLBACK EmbedDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}


// --------- Implémentation révisée fonctionnelle ---------

struct EmbedCtx {
    std::wstring text;
};

static INT_PTR CALLBACK EmbedDlgProc2(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EmbedCtx* ctx = nullptr;
    switch (msg) {
    case WM_INITDIALOG:
        ctx = reinterpret_cast<EmbedCtx*>(lParam);
        SetDlgItemText(hDlg, ID_EDIT_MESSAGE, L"");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            wchar_t buf[4096];
            GetDlgItemText(hDlg, ID_EDIT_MESSAGE, buf, 4096);
            ctx->text = buf;
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void DialogEmbedMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info) {
    if (!pixels || !info) return;

    EmbedCtx ctx;
    INT_PTR r = DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_EMBED), hwnd, EmbedDlgProc2, (LPARAM)&ctx);
    if (r != IDOK) return;

    std::string utf8 = WideToUtf8(ctx.text);

    const size_t byteSize = (size_t)info->bmiHeader.biWidth * 4ull * (size_t)abs(info->bmiHeader.biHeight);
    if (!EmbedLSB(pixels, byteSize, utf8)) {
        MessageBox(hwnd, L"Message trop long pour cette image (capacité insuffisante).", L"Erreur", MB_ICONERROR);
        return;
    }

    InvalidateRect(hwnd, NULL, FALSE);
    MessageBox(hwnd, L"Message intégré avec succès.", L"OK", MB_OK | MB_ICONINFORMATION);
}

void DialogExtractMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info) {
    if (!pixels || !info) return;
    const size_t byteSize = (size_t)info->bmiHeader.biWidth * 4ull * (size_t)abs(info->bmiHeader.biHeight);

    std::string utf8 = ExtractLSB(pixels, byteSize);
    if (utf8.empty()) {
        MessageBox(hwnd, L"Aucun message détecté.", L"Extraction", MB_OK | MB_ICONWARNING);
        return;
    }
    std::wstring w = Utf8ToWide(utf8);
    MessageBox(hwnd, w.c_str(), L"Message extrait", MB_OK | MB_ICONINFORMATION);
}
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowAboutDialog(HWND hwnd)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc, 0);
}

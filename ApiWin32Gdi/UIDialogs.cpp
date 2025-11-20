#include "UIDialogs.h"
#include <commdlg.h>
#include <string>

#include "resource.h"

// Conversion Wide (wstring) -> UTF-8 (std::string)
// Utilisé pour encoder le message saisi par l'utilisateur avant
// d'appeler l'engine de stéganographie (qui travaille en bytes/UTF-8).
static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    // WideCharToMultiByte : première passe pour connaître la taille nécessaire.
    int needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(needed, '\0');
    // Deuxième passe : conversion effective (n'écrit pas le '\0' final car on ne demande pas la taille +1).
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], needed, nullptr, nullptr);

    return out;
}

// Conversion UTF-8 -> Wide (wstring)
// Utilisé pour afficher dans MessageBox (qui attend wchar_t* en Unicode).
static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], needed);

    return out;
}

// Procédure de dialogue basique (version initiale / minimaliste).
// Ici on ne fait que gérer OK/Cancel.
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
// On utilise ici une petite structure de contexte pour récupérer
// le texte entré dans la boîte de dialogue.
struct EmbedCtx {
    std::wstring text; // texte saisi (wide) par l'utilisateur
};

// Procédure de dialogue améliorée : récupère le texte dans un contrôle (ID_EDIT_MESSAGE).
static INT_PTR CALLBACK EmbedDlgProc2(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static EmbedCtx* ctx = nullptr; // stockage temporaire du contexte pour la durée du dialog
    switch (msg) {
    case WM_INITDIALOG:
        ctx = reinterpret_cast<EmbedCtx*>(lParam);
        // Initialiser la zone d'édition à vide
        SetDlgItemText(hDlg, ID_EDIT_MESSAGE, L"");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            // Récupère le texte saisi (limité à 4096 wchar_t ici)
            wchar_t buf[4096];
            GetDlgItemText(hDlg, ID_EDIT_MESSAGE, buf, 4096);
            ctx->text = buf; // copie dans le contexte fourni par l'appelant
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

// Affiche la boîte de dialogue d'intégration, convertit le texte en UTF-8,
void DialogEmbedMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info) {
    if (!pixels || !info) return;

    EmbedCtx ctx;
    INT_PTR r = DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_EMBED), hwnd, EmbedDlgProc2, (LPARAM)&ctx);
    if (r != IDOK) return; // si l'utilisateur annule, on sort

    // Conversion en UTF-8 (l'engine attend une std::string en UTF-8)
    std::string utf8 = WideToUtf8(ctx.text);

    // Calcul de la taille en octets des pixels (width * 4 * height)
    const size_t byteSize = (size_t)info->bmiHeader.biWidth * 4ull * (size_t)abs(info->bmiHeader.biHeight);

    // Appel de l'engine : si le message est trop long, on affiche une alerte.
    if (!EmbedLSB(pixels, byteSize, utf8)) {
        MessageBox(hwnd, L"Message trop long pour cette image (capacité insuffisante).", L"Erreur", MB_ICONERROR);
        return;
    }

    // Forcer le redessin (InvalidateRect) pour que le rendu prenne en compte
    // les pixels modifiés (on ne demande pas d'effacement, le rendu gère le fond).
    InvalidateRect(hwnd, NULL, FALSE);
    MessageBox(hwnd, L"Message intégré avec succès.", L"OK", MB_OK | MB_ICONINFORMATION);
}

// Extrait un message (si présent) en appelant ExtractLSB, puis l'affiche.
// Si aucun message trouvé, avertit l'utilisateur.
void DialogExtractMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info) {
    if (!pixels || !info) return;
    const size_t byteSize = (size_t)info->bmiHeader.biWidth * 4ull * (size_t)abs(info->bmiHeader.biHeight);

    std::string utf8 = ExtractLSB(pixels, byteSize);
    if (utf8.empty()) {
        MessageBox(hwnd, L"Aucun message détecté.", L"Extraction", MB_OK | MB_ICONWARNING);
        return;
    }
    // Convertir en wide pour l'afficher
    std::wstring w = Utf8ToWide(utf8);
    MessageBox(hwnd, w.c_str(), L"Message extrait", MB_OK | MB_ICONINFORMATION);
}

// Procédure de la boîte "À propos"
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

// Affiche la boîte "À propos" via les ressources (IDD_ABOUT)
void ShowAboutDialog(HWND hwnd)
{
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc, 0);
}
#include <windows.h>
#include <commdlg.h>
#include "MainWindow.h"
#include "ImageManager.h"
#include "RendererGDI.h"
#include "StegEngine.h"
#include "UIDialogs.h"

static HINSTANCE g_hInstance = NULL;
static BITMAPINFO* g_pInfo = NULL;
static BYTE* g_pPixels = NULL;

// Valeurs sauvegardées pour l'état de maximisation
static bool g_isMaximized = false;
static WINDOWPLACEMENT g_prevPlacement = { sizeof(WINDOWPLACEMENT) };

// Basculer maximisation marche/arrêt (conserve la bordure et le menu de la fenêtre)
static void ToggleMaximize(HWND hwnd)
{
    if (!hwnd) return;

    if (!g_isMaximized)
    {
        // Sauvegarde la position/état actuel puis maximise
        GetWindowPlacement(hwnd, &g_prevPlacement);
        ShowWindow(hwnd, SW_MAXIMIZE);
        g_isMaximized = true;
    }
    else
    {
        // Restaure la position/état précédents
        SetWindowPlacement(hwnd, &g_prevPlacement);
        ShowWindow(hwnd, SW_RESTORE);
        g_isMaximized = false;
    }

    // Forcer un rafraîchissement complet pour éviter des visuels résiduels
    // Utilise RedrawWindow pour forcer immédiatement l'effacement du fond
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        g_hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_pPixels)
            RenderImage(hdc, g_pInfo, g_pPixels);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        // Nous gérons l'effacement de l'arrière-plan dans RenderImage (FillRect). Retourner une valeur non nulle
        // empêche l'effacement par défaut qui peut parfois laisser des artefacts lors du redimensionnement/maximisation.
        return 1;

    case WM_SIZE:
        // Forcer un redessin complet (effacement + peinture) lorsque la fenêtre change de taille,
        // cela évite la petite image résiduelle/miniature en haut à gauche après la maximisation.
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_F11)
        {
            ToggleMaximize(hwnd);
            return 0;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_OPEN: {
            OPENFILENAME ofn = {};
            wchar_t fileName[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L".bmp\0*.bmp\0";
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileName(&ofn)) {
                if (g_pPixels) {
                    delete[] g_pPixels;
                    g_pPixels = NULL;
                }
                if (g_pInfo) {
                    free(g_pInfo);
                    g_pInfo = NULL;
                }
                if (LoadBMP(fileName, g_pInfo, g_pPixels)) {
                    // Forcer l'effacement complet pour que rien de l'ancien contenu ne subsiste
                    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
                else
                    MessageBox(hwnd, L"Erreur lors du chargement du BMP.", L"Erreur", MB_ICONERROR);
            }
            break;
        }

        case ID_FILE_SAVE: {
            if (!g_pPixels || !g_pInfo) {
                MessageBox(hwnd, L"Aucune image ・enregistrer.", L"Information", MB_ICONINFORMATION);
                break;
            }

            OPENFILENAME ofn = {};
            wchar_t fileName[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L".bmp\0*.bmp\0";
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT;
            ofn.lpstrDefExt = L"bmp";

            if (GetSaveFileName(&ofn)) {
                if (SaveBMP(fileName, g_pPixels, g_pInfo))
                    MessageBox(hwnd, L"Image enregistrée avec succès !", L"Succès", MB_ICONINFORMATION);
                else
                    MessageBox(hwnd, L"Erreur lors de l'enregistrement du BMP.", L"Erreur", MB_ICONERROR);
            }
            break;
        }

        case ID_STEG_EMBED:
            if (g_pPixels)
                DialogEmbedMessage(hwnd, g_pPixels, g_pInfo);
            else
                MessageBox(hwnd, L"Aucune image chargée.", L"Erreur", MB_ICONERROR);
            break;

        case ID_STEG_EXTRACT:
            if (g_pPixels)
                DialogExtractMessage(hwnd, g_pPixels, g_pInfo);
            else
                MessageBox(hwnd, L"Aucune image chargée.", L"Erreur", MB_ICONERROR);
            break;

        case ID_HELP_ABOUT:
            ShowAboutDialog(hwnd);
            break;

        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        if (g_pPixels) {
            delete[] g_pPixels;
            g_pPixels = NULL;
        }
        if (g_pInfo) {
            free(g_pInfo);
            g_pInfo = NULL;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

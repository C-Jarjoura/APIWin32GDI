#include <windows.h>
#include <commdlg.h>
#include "MainWindow.h"
#include "ImageManager.h"
#include "RendererGDI.h"
#include "StegEngine.h"
#include "UIDialogs.h"

// Variables globales simples pour l'état de la fenêtre et l'image chargée.
static HINSTANCE g_hInstance = NULL;
static BITMAPINFO* g_pInfo = NULL;
static BYTE* g_pPixels = NULL;

// Sauvegarde de l'état pour la maximisation/restauration
static bool g_isMaximized = false;
static WINDOWPLACEMENT g_prevPlacement = { sizeof(WINDOWPLACEMENT) };

// Basculer maximisation marche/arrêt (conserve la bordure et le menu)
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
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

// Procédure principale de la fenêtre
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        // Récupération de hInstance à partir de la structure CREATESTRUCT
        g_hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        break;

    case WM_PAINT: {
        // Peinture de la fenêtre : on appelle RenderImage si on a des pixels à afficher.
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_pPixels)
            RenderImage(hdc, g_pInfo, g_pPixels);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        // Forcer un redessin complet (effacement + peinture) lors du redimensionnement
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        break;

    case WM_KEYDOWN:
        // F11 pour basculer maximisation/restauration
        if (wParam == VK_F11)
        {
            ToggleMaximize(hwnd);
            return 0;
        }
        break;

    case WM_COMMAND:
        // Gestion des commandes menu
        switch (LOWORD(wParam))
        {
        case ID_FILE_OPEN: {
            // Ouvrir un fichier BMP via la boîte standard
            OPENFILENAME ofn = {};
            wchar_t fileName[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L".bmp\0*.bmp\0";
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (GetOpenFileName(&ofn)) {
                // Libérer éventuellement l'ancienne image (attention : on supprime avant d'essayer de charger)
                if (g_pPixels) {
                    delete[] g_pPixels;
                    g_pPixels = NULL;
                }
                if (g_pInfo) {
                    free(g_pInfo);
                    g_pInfo = NULL;
                }
                // Charger la nouvelle image
                if (LoadBMP(fileName, g_pInfo, g_pPixels)) {
                    // Forcer effacement complet du client pour afficher la nouvelle image proprement
                    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
                else
                    MessageBox(hwnd, L"Erreur lors du chargement du BMP.", L"Erreur", MB_ICONERROR);
            }
            break;
        }

        case ID_FILE_SAVE: {
            // Sauvegarder l'image courante (si existe)
            if (!g_pPixels || !g_pInfo) {
                MessageBox(hwnd, L"Aucune image à enregistrer.", L"Information", MB_ICONINFORMATION);
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
            // Intégrer un message dans l'image (si image chargée)
            if (g_pPixels)
                DialogEmbedMessage(hwnd, g_pPixels, g_pInfo);
            else
                MessageBox(hwnd, L"Aucune image chargée.", L"Erreur", MB_ICONERROR);
            break;

        case ID_STEG_EXTRACT:
            // Extraire et afficher le message (si image chargée)
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
        // Libération des ressources lors de la fermeture
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
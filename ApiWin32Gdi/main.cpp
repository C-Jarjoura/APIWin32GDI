#define UNICODE
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "MainWindow.h"
#include "resource.h"   

// ----------------------------------------------------
// Point d’entrée principal de l’application Win32
// ----------------------------------------------------
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    // Définition de la classe de fenêtre principale
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"StegToolMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    // Chargement de l'icône principale depuis les ressources (ID défini dans Resource.h)
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON)); // 👈 Icône principale

    // Enregistrement de la classe
    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, L"Erreur : Enregistrement de la classe échoué.", L"Erreur", MB_ICONERROR);
        return 0;
    }

    // Création de la fenêtre principale
    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"Outil de stéganographie BMP",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd)
    {
        MessageBox(NULL, L"Erreur : Création de la fenêtre échouée.", L"Erreur", MB_ICONERROR);
        return 0;
    }

    // Chargement du menu depuis les ressources (fichier .rc doit référencer IDR_MAINMENU)
    HMENU hMenuBar = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU));
    if (hMenuBar)
        SetMenu(hwnd, hMenuBar);
    else
        MessageBox(hwnd, L"Impossible de charger le menu.", L"Avertissement", MB_ICONWARNING);

    // Affichage de la fenêtre
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Boucle principale de messages Windows
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
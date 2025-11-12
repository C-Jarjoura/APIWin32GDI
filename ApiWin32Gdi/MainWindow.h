#pragma once
#include <windows.h>

// Identifiants des commandes du menu
#define ID_FILE_OPEN        101
#define ID_FILE_SAVE        102
#define ID_FILE_EXIT        103
#define ID_STEG_EMBED       201
#define ID_STEG_EXTRACT     202
#define ID_HELP_ABOUT       301
#define IDR_MAINMENU        400

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

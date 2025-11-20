#pragma once
#include <windows.h>
#include "StegEngine.h"
#include "ImageManager.h"

// Déclarations des fonctions de dialogues UI utilisées par l'application.
// Ces fonctions sont définies dans UIDialogs.cpp et s'occupent de
// - afficher une boîte de dialogue pour saisir un message à intégrer,
// - afficher une boîte de dialogue pour extraire et afficher un message,
// - afficher la boîte "À propos".
//
// Paramètres communs : hwnd = handle de la fenêtre parente,
// pixels = pointeur vers les pixels (format BGRA 32 bits top-down),
// info = BITMAPINFO décrivant l'image (doit correspondre aux pixels).

void DialogEmbedMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info);
void DialogExtractMessage(HWND hwnd, BYTE* pixels, BITMAPINFO* info);
void ShowAboutDialog(HWND hwnd);
🖼️ Outil de Stéganographie BMP - Win32 API

## Aperçu
Application Windows native écrite en C/C++ (Visual Studio 2022) exploitant exclusivement les API Win32 et GDI. Elle permet de dissimuler et de récupérer des messages texte dans des images bitmap en utilisant la méthode LSB (Least Significant Bit).

## Prérequis
- Windows 10 ou 11 (x64)
- Visual Studio 2022 avec le workload « Desktop development with C++ »
- Aucun framework ou bibliothèque externe : seules les API Win32/GDI sont utilisées

## Installation & Compilation
////Executable//////
1.Telecharger l'executable `ApiWin32Gdi.exe/`
2.Lancer l'executable

/////Visual Studio/////
1. Cloner ou copier le dépôt sur votre machine Windows.
2. Ouvrir `ApiWin32Gdi.sln` avec Visual Studio 2022.
3. Sélectionner la configuration désirée (`Debug` ou `Release`) et la plateforme `x64`.
4. Compiler via **Build → Build Solution** (`Ctrl+Shift+B`).
5. L’exécutable `ApiWin32Gdi.exe` est généré dans `x64/Debug` ou `x64/Release`.

## Formats d’image supportés
- BMP non compressés 24 bits ou 32 bits.
- Les images sont converties en DIB 32 bits BGRA top-down à l’ouverture pour garantir la cohérence des opérations de stéganographie.
- Le support PNG est optionnel et **n’est pas implémenté** dans cette version.

## Procédure d’utilisation
### Intégrer un message
1. Menu **Fichier → Ouvrir…** et choisir une image BMP.
2. Menu **Stéganographie → Intégrer un message…**.
3. Saisir le texte à cacher dans la boîte de dialogue puis valider.
4. Menu **Fichier → Enregistrer sous…** pour sauvegarder l’image contenant le message.

### Extraire un message
1. Menu **Fichier → Ouvrir…** et sélectionner une image BMP contenant un message caché.
2. Menu **Stéganographie → Extraire un message…**.
3. Le texte décodé s’affiche dans la boîte de dialogue.

## Capacités & limites
- La capacité maximale dépend du nombre de pixels : chaque pixel peut stocker 3 bits utiles.
- L’application ajoute un en-tête `MAGIC + longueur` avant les données pour identifier la présence d’un message.
- Les images trop petites ou corrompues peuvent ne pas accepter l’intégration d’un message complet.

## Ressources utiles
- Dossier `assets/` avec quelques images utiles pour tester l'application. 
- Le code source complet se trouve dans le dossier `ApiWin32Gdi/`.


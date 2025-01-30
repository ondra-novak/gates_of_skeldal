#include "../save_folder.h"
#include "../error.h"
#include <iostream>
#include <filesystem>
#include <windows.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// Pro získání uživatelské složky
namespace fs = std::filesystem;

// Funkce pro získání složky "Saved Games"
std::string getSavedGamesDirectory() {
    PWSTR path = nullptr;
    // Použití identifikátoru složky FOLDERID_SavedGames
    if (SUCCEEDED(SHGetKnownFolderPathA(FOLDERID_SavedGames, 0, NULL, &path))) {
        fs::path savedGamesPath(path);
        CoTaskMemFree(path); // Uvolnění paměti
        return savedGamesPath / SAVEGAME_FOLDERNAME
    } else {
        display_error("Failed to retrieve FOLDEROD_SavedGames");
        abort();
    }
}
const char *get_default_savegame_directory() {

    static std::string dir = get_default_savgetSavedGamesDirectoryegame_dir();
    return dir.c_str();

}

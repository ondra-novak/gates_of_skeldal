#include "../platform.h"
#include "../save_folder.h"
#include "../error.h"
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <ShlObj.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace fs = std::filesystem;

std::string getSavedGamesDirectory() {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path))) {
        fs::path savedGamesPath(path);
        CoTaskMemFree(path); 
        return (savedGamesPath / SAVEGAME_FOLDERNAME).string();
    } else {
        display_error("Failed to retrieve FOLDEROD_SavedGames");
        abort();
    }
}
const char *get_default_savegame_directory() {

    static std::string dir = getSavedGamesDirectory();
    return dir.c_str();

}

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

        // Převod na UTF-8 std::string
        std::u8string utf8 = (savedGamesPath / SAVEGAME_FOLDERNAME).u8string();
        return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
    } else {
        display_error("Failed to retrieve FOLDERID_SavedGames");
        abort();
    }
}

const char *get_default_savegame_directory() {

    static std::string dir = getSavedGamesDirectory();
    return dir.c_str();

}

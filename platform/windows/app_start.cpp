#include "../../game/skeldal.h"
#include "../getopt.h"
#include "../platform.h"
#include "../error.h"
#include <iostream>
#include <string>
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <shellapi.h>

void show_help(std::ostream &out, const char *arg0) {
    out <<
            "Brany Skeldalu (Gates of Skeldal) portable game player\n"
            "Copyright (c) 2025 Ondrej Novak. All rights reserved.\n\n"
            "This work is licensed under the terms of the MIT license.\n"
            "For a copy, see <https://opensource.org/licenses/MIT>.\n"
            "\n"
            "Usage:";
    out << arg0 << " [-f <file>] [-a <file>] [-l <lang>] [-s <dir>] [-h]\n\n";
    out << "-f <file>       path to configuration file\n"
           "-a <adv>        path for adventure file (.adv)\n"
           "-p <file>       patch data with custom DDL\n"
           "-l <lang>       set language (cz|en)\n"
           "-s <directory>  generate string-tables (for localization) and exit\n"
           "-c <host:port>  connect to host:port for remote control (mapedit)\n"
           "-L              run launcher - select workshop adventure\n"
           "-h              this help\n";
}

void show_help_short(std::ostream &out) {
    out << "Use -h for help\n";
}


int main(int argc, char **argv) {
    std::ostringstream console;
    SKELDAL_CONFIG cfg = {SKELDALINI,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    for (int optchr = -1; (optchr = getopt(argc, argv, "hLf:a:s:l:p:c:P:")) != -1; ) {
        switch (optchr) {
            case 'h': show_help(console,argv[0]);break;
            case 'f': cfg.config_path = optarg;break;
            case 'a': cfg.adventure_path = optarg;break;
            case 'p': cfg.patch_file = optarg; break;
            case 'l': cfg.langddl = optarg;break;
            case 'c': cfg.sse_hostport = optarg;break;
            case 'P': cfg.workshop_publish = optarg;break;
            default: show_help_short(console);
                     return 1;
        }
    }
    cfg.show_error = [](const char *txt) {
        char buff[MAX_PATH];
        GetModuleFileNameA(NULL,buff,MAX_PATH);
        MessageBoxA(NULL,txt,buff, MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL|MB_APPLMODAL);
        ExitProcess(1);
    };

    cfg.short_help = []{};

    if (!check_file_exists(cfg.config_path)) {
        console << "ERROR: A configuration file was not found:\n\n" << cfg.config_path<< "\n\n";
        show_help(console, argv[0]);
    }

    {
        std::string msg = console.str();
        if (!msg.empty()) {
            cfg.show_error(msg.c_str());
            return 1;
        }
    }

    try {
        return skeldal_entry_point(&cfg);
    } catch (const std::exception &e) {
        cfg.show_error(exception_to_string(e).c_str());
        return 1;
    } catch (...) {
        cfg.show_error("Uknown error or crash");
        return 1;
    }

    return 0;

}

int __stdcall WinMain(HINSTANCE,HINSTANCE ,LPSTR, INT) {

    int argc;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);

    char **argv = (char **)alloca(sizeof(char *) * argc);
    for (int i = 0; i < argc; ++i) {
        DWORD need = WideCharToMultiByte(CP_UTF8,0,szArglist[i],wcslen(szArglist[i]),NULL,NULL,NULL,FALSE)+1;
        argv[i] = (char *)alloca(sizeof(char) * need);
        WideCharToMultiByte(CP_UTF8,0,szArglist[i],wcslen(szArglist[i]),argv[i],need,NULL,FALSE);
        argv[i][need-1] = 0;
    }
    GlobalFree(szArglist);
    return main(argc, argv);
}


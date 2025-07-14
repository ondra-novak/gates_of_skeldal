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
    std::string config_name = SKELDALINI;
    std::string adv_config_file;
    std::string gen_stringtable_path;
    std::string lang;
    std::string patch;
    std::string sse_hostport;
    bool launcher = false;
    std::ostringstream console;
    for (int optchr = -1; (optchr = getopt(argc, argv, "hLf:a:s:l:c:p:")) != -1; ) {
        switch (optchr) {
            case 'f': config_name = optarg;break;
            case 'a': adv_config_file = optarg;break;
            case 'h': show_help(console, argv[0]);break;
            case 'p': patch = optarg; break;
            case 'l': lang = optarg;break;
            case 'L': launcher = true;break;

            case 's': gen_stringtable_path = optarg;break;
            case 'c': sse_hostport = optarg;break;
            default:  show_help_short(console);break;
        }
    }

    if (!check_file_exists(config_name.c_str())) {
        console << "ERROR: A configuration file was not found:\n\n" << config_name << "\n\n";
        show_help(console, argv[0]);
    }

    SKELDAL_CONFIG cfg = {};
    cfg.short_help = []{};
    cfg.show_error = [](const char *txt) {
        char buff[MAX_PATH];
        GetModuleFileNameA(NULL,buff,MAX_PATH);
        MessageBoxA(NULL,txt,buff, MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL|MB_APPLMODAL);
        ExitProcess(1);
    };
    cfg.adventure_path = adv_config_file.empty()?NULL:adv_config_file.c_str();
    cfg.config_path = config_name.c_str();
    cfg.lang_path = lang.empty()?NULL:lang.c_str();
    cfg.patch_file = patch.empty()?NULL:patch.c_str();
    cfg.sse_hostport = sse_hostport.empty()?NULL:sse_hostport.c_str();
    cfg.launcher = launcher?1:0;

    {
        std::string msg = console.str();
        if (!msg.empty()) {
            cfg.show_error(msg.c_str());
            return 1;
        }
    }

    try {

        if (!gen_stringtable_path.empty()) {
            skeldal_gen_string_table_entry_point(&cfg, gen_stringtable_path.c_str());
            return 0;
        } else {
            return skeldal_entry_point(&cfg);
        }

    } catch (const std::exception &e) {
        cfg.show_error(exception_to_string(e).c_str());
        return 1;
    }
    catch (...) {
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


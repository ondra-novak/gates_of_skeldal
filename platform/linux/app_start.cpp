#include "../../game/skeldal.h"
#include "../getopt.h"
#include "../platform.h"
#include "../error.h"
#include <iostream>
#include <string>

void show_help(const char *arg0) {
    printf(
            "Brany Skeldalu (Gates of Skeldal) portable game player\n"
            "Copyright (c) 2025 Ondrej Novak. All rights reserved.\n\n"
            "This work is licensed under the terms of the MIT license.\n"
            "For a copy, see <https://opensource.org/licenses/MIT>.\n"
            "\n"
            "Usage:"
    );
    printf("%s [-f <file>] [-a <file>] [-p <file> ] [-l <langddl>] [-s <dir>] [-h]\n\n", arg0);

    printf("-f <file>       path to configuration file\n"
           "-a <adv>        path for adventure file (.adv) (deprecated)\n"
           "-p <file>       patch data with custom DDL (editor)\n"
           "-l <langddl>    use language DDL for -p (default is CZ.DDL)\n"
           "-c <host:port>  connect to host:port for remote control (mapedit)\n"
           "-h              this help\n");
    exit(1);
}

void show_help_short() {
    printf("Use -h for help\n");
}


int main(int argc, char **argv) {
    std::string config_name = SKELDALINI;
    std::string adv_config_file;
    std::string patch;
    std::string lang;
    std::string sse_hostport;
    for (int optchr = -1; (optchr = getopt(argc, argv, "hLf:a:s:l:p:c:")) != -1; ) {
        switch (optchr) {
            case 'f': config_name = optarg;break;
            case 'a': adv_config_file = optarg;break;
            case 'h': show_help(argv[0]);break;
            case 'p': patch = optarg; break;
            case 'l': lang = optarg;break;
            case 'c': sse_hostport = optarg;break;
            default: show_help_short();
                     return 1;
        }
    }

    SKELDAL_CONFIG cfg;
    cfg.short_help = show_help_short;
    cfg.show_error = [](const char *txt) {
        std::cerr << "ERROR: " << txt << std::endl;
        abort();
    };
    cfg.adventure_path = adv_config_file.empty()?NULL:adv_config_file.c_str();
    cfg.config_path = config_name.c_str();
    cfg.langddl = lang.empty()?NULL:lang.c_str();
    cfg.patch_file = patch.empty()?NULL:patch.c_str();
    cfg.sse_hostport = sse_hostport.empty()?NULL:sse_hostport.c_str();
    try {
        return skeldal_entry_point(&cfg);
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << exception_to_string(e) << std::endl;
        return 1;
    }

    return 0;

}


#include "../../game/skeldal.h"
#include "../getopt.h"
#include "../platform.h"
#include "../error.h"
#include <iostream>

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

    SKELDAL_CONFIG cfg = {SKELDALINI,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
    for (int optchr = -1; (optchr = getopt(argc, argv, "hLf:a:s:l:p:c:P:")) != -1; ) {
        switch (optchr) {
            case 'h': show_help(argv[0]);break;
            case 'f': cfg.config_path = optarg;break;
            case 'a': cfg.adventure_path = optarg;break;
            case 'p': cfg.patch_file = optarg; break;
            case 'l': cfg.langddl = optarg;break;
            case 'c': cfg.sse_hostport = optarg;break;
            case 'P': cfg.workshop_publish = optarg;break;
            default: show_help_short();
                     return 1;
        }
    }

    cfg.short_help = show_help_short;
    cfg.show_error = [](const char *txt) {
        std::cerr << "ERROR: " << txt << std::endl;
        abort();
    };
    try {
        return skeldal_entry_point(&cfg);
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << exception_to_string(e) << std::endl;
        return 1;
    }

    return 0;

}


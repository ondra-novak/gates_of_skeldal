#include "../../game/skeldal.h"
#include "../getopt.h"
#include "../platform.h"
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
    printf("%s [-f <file>] [-a <file>] [-l <lang>] [-s <dir>] [-h]\n\n", arg0);

    printf("-f <file>       path to configuration file\n"
           "-a <adv>        path for adventure file (.adv)\n"
           "-l <lang>       set language (cz|en)\n"
           "-s <directory>  generate string-tables (for localization) and exit\n"
           "-h              this help\n");
    exit(1);
}

void show_help_short() {
    printf("Use -h for help\n");
}


int main(int argc, char **argv) {
    std::string config_name = SKELDALINI;
    std::string adv_config_file;
    std::string gen_stringtable_path;
    std::string lang;
    for (int optchr = -1; (optchr = getopt(argc, argv, "hf:a:s:l:")) != -1; ) {
        switch (optchr) {
            case 'f': config_name = optarg;break;
            case 'a': adv_config_file = optarg;break;
            case 'h': show_help(argv[0]);break;
            case 'l': lang = optarg;break;
            case 's': gen_stringtable_path = optarg;break;
            default: show_help_short();
                     return 1;
        }
    }

    SKELDAL_CONFIG cfg;
    cfg.short_help = show_help_short;
    cfg.show_error = [](const char *txt) {
        std::cerr << "ERROR: " << txt << std::endl;
    };
    cfg.adventure_path = adv_config_file.empty()?NULL:adv_config_file.c_str();
    cfg.config_path = config_name.c_str();
    cfg.lang_path = lang.empty()?NULL:lang.c_str();
    try {

        if (!gen_stringtable_path.empty()) {
            skeldal_gen_string_table_entry_point(&cfg, gen_stringtable_path.c_str());
            return 0;
        } else {
            return skeldal_entry_point(&cfg);
        }

    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;

}


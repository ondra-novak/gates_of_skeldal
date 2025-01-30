#include "config.h"
#include "platform.h"
#include <fstream>
#include <map>
#include <string>
#include <string_view>
typedef struct ini_config_tag {

    using Section = std::map<std::string, std::string, std::less<>>;
    using Config = std::map<std::string, Section , std::less<>>;
    Config data;

} INI_CONFIG;



// Trim whitespace z obou stran pro string_view
static inline std::string_view trim(std::string_view str) {
    const size_t first = str.find_first_not_of(" \t");
    if (first == std::string_view::npos) return {};
    const size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Parser INI souboru
template<std::invocable<std::string_view, std::string_view, std::string_view> Callback >
void parseIniStream(std::istream& input, Callback &&callback) {
    std::string line;
    std::string currentSection;

    while (std::getline(input, line)) {
        std::string_view line_view = trim(line);

        // Ignoruj prázdné řádky nebo komentáře
        if (line_view.empty() || line_view.front() == '#') continue;

        // Detekce sekce [sekce]
        if (line_view.front() == '[' && line_view.back() == ']') {
            currentSection = std::string(trim(line_view.substr(1, line_view.size() - 2)));
        } else {
            // Zpracuj klíč = hodnota
            size_t eqPos = line_view.find('=');
            if (eqPos != std::string_view::npos) {
                std::string_view key_view = trim(line_view.substr(0, eqPos));
                std::string_view value_view = trim(line_view.substr(eqPos + 1));
                callback(currentSection, std::string(key_view), std::string(value_view));
            }
        }
    }
}


const INI_CONFIG* ini_open(const char *filename) {
    INI_CONFIG *c = new INI_CONFIG;

    std::fstream input(filename);
    if (!input) return c;
    parseIniStream(input, [&](std::string_view section, std::string_view key, std::string_view value) {
        INI_CONFIG::Config::iterator iter = c->data.find(section);
        if (iter == c->data.end()) {
            iter = c->data.emplace(std::string(section), INI_CONFIG::Section()).first;
        }
        iter->second.emplace(std::string(key), std::string(value));
    });
    return c;
}

void ini_close(const INI_CONFIG *config) {
    delete config;
}

const INI_CONFIG_SECTION* ini_section_open(const INI_CONFIG *cfg, const char *section) {
    auto iter = cfg->data.find(std::string_view(section));
    if (iter == cfg->data.end()) return NULL;
    else return reinterpret_cast<const INI_CONFIG_SECTION *>(&iter->second);
}

const char* ini_find_key(const INI_CONFIG_SECTION *section,
        const char *key) {
    if (section == NULL) return NULL;
    const INI_CONFIG::Section *s = reinterpret_cast<const INI_CONFIG::Section *>(section);
    auto iter = s->find(std::string_view(key));
    if (iter == s->end()) return NULL;
    return iter->second.c_str();
}


long ini_get_value_int(const char *value, int *conv_ok) {
    char *out = NULL;
    if (value != NULL) {
        long ret = strtol(value, &out, 10);
        if (*out == 0) {
            if (*conv_ok) *conv_ok = 1;
            return ret;
        }
    }
    if (*conv_ok) *conv_ok = 0;
    return -1;

}

double ini_get_value_double(const char *value, int *conv_ok) {
    char *out = NULL;
    if (value != NULL) {
        double ret = strtod(value, &out);
        if (*out == 0) {
            if (*conv_ok) *conv_ok = 1;
            return ret;
        }
    }
    if (*conv_ok) *conv_ok = 0;
    return -1;
}

int ini_get_value_boolean(const char *value) {
    int r = -1;
    if (value != NULL) {
        if (stricmp(value, "true") == 0
                || stricmp(value, "1") == 0
                || stricmp(value, "on") == 0
                || stricmp(value, "yes") == 0) {
            r = 1;
        } else if (stricmp(value, "false") == 0
                || stricmp(value, "0") == 0
                || stricmp(value, "off") == 0
                || stricmp(value, "no") == 0) {
            r = 0;
        } else {
            r = -1;
        }
    }
    return r;
}

const char* ini_get_string(
        const INI_CONFIG_SECTION *section, const char *key, const char *defval) {
    const char *k = ini_find_key(section, key);
    return k?k:defval;
}

long ini_get_int(const INI_CONFIG_SECTION *section, const char *key, long defval) {
    const char *k = ini_find_key(section, key);
    if (k) {
        int ok;
        long r = ini_get_value_int(k, &ok);
        return ok?r:defval;
    }
    return defval;
}

int ini_get_double(const INI_CONFIG_SECTION *section, const char *key,
        double defval) {
    const char *k = ini_find_key(section, key);
    if (k) {
        int ok;
        double r = ini_get_value_double(k, &ok);
        return ok?r:defval;
    }
    return defval;
}

int ini_get_boolean(const INI_CONFIG_SECTION *section, const char *key,
        int defval) {
    const char *k = ini_find_key(section, key);
    if (k) {
        int r = ini_get_value_boolean(k);
        return r>=0?r:defval;
    }
    return defval;
}

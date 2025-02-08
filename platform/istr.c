#include "platform.h"

#include <ctype.h>
int istrcmp(const char *a, const char *b) {
    while (*a && *b) {
        char ca = toupper(*a);
        char cb = toupper(*b);
        if (ca<cb) return -1;
        if (ca>cb) return 1;
        ++a;
        ++b;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

void strupper(char *c) {
    while (*c) {
        *c = toupper(*c);
        ++c;
    }
}


// Funkce pro přeskakování bílých znaků
static const char *skip_whitespace(const char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

// Funkce porovnávající dvě slova bez ohledu na velikost písmen
static int compare_words(const char *word1, const char *word2, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (tolower((unsigned char)word1[i]) != tolower((unsigned char)word2[i])) {
            return 0;
        }
    }
    return 1;
}

// Hlavní vyhledávací funkce
int imatch(const char *text, const char *hledany) {
    // Rozdělení hledaného podřetězce na slova
    const char *text_ptr = text;
    const char *search_ptr = hledany;

    // Iteruj přes slova hledaného podřetězce
    while (*search_ptr) {
        search_ptr = skip_whitespace(search_ptr);

        // Pokud jsme na konci hledaného textu, ukončíme
        if (*search_ptr == '\0') {
            break;
        }

        // Najdi konec aktuálního slova
        const char *search_word_end = search_ptr;
        while (*search_word_end && !isspace((unsigned char)*search_word_end)) {
            search_word_end++;
        }

        size_t search_word_len = search_word_end - search_ptr;

        // Hledání slova v textu
        int found = 0;
        while (*text_ptr) {
            text_ptr = skip_whitespace(text_ptr);
            if (*text_ptr == '\0') {
                break;
            }

            const char *text_word_end = text_ptr;
            while (*text_word_end && !isspace((unsigned char)*text_word_end)) {
                text_word_end++;
            }

            size_t text_word_len = text_word_end - text_ptr;

            // Porovnání aktuálního slova
            if (text_word_len >= search_word_len) {
                for (int i = text_word_len  - search_word_len; i >= 0; --i) {
                    if (compare_words(text_ptr+i, search_ptr, search_word_len)) {
                        found = 1;
                        text_ptr = text_word_end; // Pokračujeme za slovem
                        break;
                    }
                }
                if (found) break;
            }

            text_ptr = text_word_end;
        }

        if (!found) {
            return 0; // Nenalezeno
        }

        search_ptr = search_word_end;
    }

    return 1; // Všechny části byly nalezeny ve správném pořadí
}

const char *strcopy_n(char *target, const char *source, int target_size) {
    const char *ret = target;
    while (target_size>1 && *source) {
        *target++ = *source++;
        --target_size;
    }
    *target = 0;
    return ret;
}

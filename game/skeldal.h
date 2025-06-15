#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    void (*short_help)();
    void (*show_error)(const char *text);

    const char *adventure_path;
    const char *config_path;
    const char *lang_path;
    const char *patch_file;


} SKELDAL_CONFIG;

int skeldal_entry_point(const SKELDAL_CONFIG *cfg);
int skeldal_gen_string_table_entry_point(const SKELDAL_CONFIG *cfg, const char *save_path);


#ifdef __cplusplus
}
#endif

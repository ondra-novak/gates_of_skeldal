#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

    const char *config_path;
    const char *adventure_path;
    const char *langddl;
    const char *patch_file;
    const char *workshop_publish;
    const char *sse_hostport;

    void (*short_help)();
    void (*show_error)(const char *text);


} SKELDAL_CONFIG;

int skeldal_entry_point(const SKELDAL_CONFIG *cfg);
int skeldal_gen_string_table_entry_point(const SKELDAL_CONFIG *cfg, const char *save_path);


#ifdef __cplusplus
}
#endif

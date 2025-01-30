#ifdef __cplusplus
extern "C" {
#endif


typedef struct ini_config_tag INI_CONFIG;
typedef struct ini_config_section_tag INI_CONFIG_SECTION;

///Opens config, returns handle
INI_CONFIG *ini_open(const char *filename);
///closes the config and frees memory
void ini_close(INI_CONFIG *config);

///Opens section
/**
 *
 * @param cfg ini config
 * @param section section name
 * @return if section exists, return handle to section otherwise returns NULL.
 * The section don't need to be closed, however it is tied to INI_CONFIG handle, so
 * one the config is closed, the section becomes invalid
 */
const INI_CONFIG_SECTION *ini_section_open(const INI_CONFIG *cfg, const char *section);
///Finds key in section
/**
 * @param section handle to  section
 * @param key name of key
 * @return handle of key if found, or NULL if not. The returned pointer points
 * to string representation of the value (so you can use it as string value)
 */
const char *ini_find_key(const INI_CONFIG_SECTION *section, const char *key);
///retrieves value as integer
/**
 * @param value found value
 * @param conv_error (optional, can be NULL) - retrieves 1 if conversion succes, 0 if failed
 * @return if conversion successed, returns value, otherwise returns -1
 */
long ini_get_value_int(const char *value, int *conv_ok);
///retrieves value as double
/**
 * @param value found value
 * @param conv_error (optional, can be NULL) - retrieves 1 if conversion succes, 0 if failed
 * @return if conversion successed, returns value, otherwise returns -1
 */
double ini_get_value_double(const char *value, int *conv_ok);
///retrieves value as boolean
/**
 * @param value found value
 * @param conv_error (optional, can be NULL) - retrieves 1 if conversion succes, 0 if failed
 * @retval 1 value is true, nonzero, yes or on
 * @retval 0 value is false, zero, no, or off
 * @retval -1 cannot convert value
 *
 */
int ini_get_value_boolean(const char *value);

INI_CONFIG_SECTION* ini_create_section(INI_CONFIG *cfg, const char *section_name);

INI_CONFIG *ini_create_empty(void);

void ini_store_to_file(const INI_CONFIG *cfg, const char *filename);


///Replace existing key with different value
void ini_replace_key(INI_CONFIG_SECTION *section, const char *key, const char *value);

///retrieve string from ini
/**
 * @param section section
 * @param key key
 * @param defval default value
 * @return if key doesn't exits or parse error, returns defval, otherwise returns value
 */
const char *ini_get_string(const INI_CONFIG_SECTION *section, const char *key, const char *defval);
///retrieve int from ini
/**
 * @param section section
 * @param key key
 * @param defval default value
 * @return if key doesn't exits or parse error, returns defval, otherwise returns value
 */
long ini_get_int(const INI_CONFIG_SECTION *section, const char *key, long defval);
///retrieve double from ini
/**
 * @param section section
 * @param key key
 * @param defval default value
 * @return if key doesn't exits or parse error, returns defval, otherwise returns value
 */
int ini_get_double(const INI_CONFIG_SECTION *section, const char *key, double defval);
///retrieve boolean from ini
/**
 * @param section section
 * @param key key
 * @param defval default value
 * @return if key doesn't exits or parse error, returns defval, otherwise returns value
 */
int ini_get_boolean(const INI_CONFIG_SECTION *section, const char *key, int defval);





#ifdef __cplusplus
}
#endif

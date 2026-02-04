#include "hami_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_LINE_LEN 1024
#define MAX_VALUE_LEN 512

extern size_t get_limit_from_env(const char* env_name);

static bool g_config_loaded_from_file = false;

static char* trim_whitespace(char* str) {
    char* end;

    /* Trim leading space */
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    return str;
}

static int count_indent(const char* line) {
    int indent = 0;
    while (line[indent] == ' ' || line[indent] == '\t') {
        indent++;
    }
    return indent;
}

static bool extract_key_value(const char* line, char* key, char* value) {
    const char* colon = strchr(line, ':');
    if (colon == NULL) {
        return false;
    }

    /* Copy key */
    size_t key_len = colon - line;
    if (key_len >= MAX_VALUE_LEN) {
        key_len = MAX_VALUE_LEN - 1;
    }
    strncpy(key, line, key_len);
    key[key_len] = '\0';

    /* Copy value (if any) */
    const char* val_start = colon + 1;
    if (*val_start != '\0') {
        strcpy(value, trim_whitespace((char*)val_start));
    } else {
        value[0] = '\0';
    }

    return true;
}

static bool is_comment(const char* line) {
    const char* trimmed = trim_whitespace((char*)line);
    return *trimmed == '#';
}

static bool parse_bool(const char* value, bool default_val) {
    char* val = trim_whitespace((char*)value);
    if (strcasecmp(val, "true") == 0 || strcasecmp(val, "yes") == 0 || strcmp(val, "1") == 0) {
        return true;
    }
    if (strcasecmp(val, "false") == 0 || strcasecmp(val, "no") == 0 || strcmp(val, "0") == 0) {
        return false;
    }
    return default_val;
}

static size_t parse_memory_size(const char* value) {
    char* val = trim_whitespace((char*)value);
    char* endptr;
    size_t size = (size_t)strtoul(val, &endptr, 10);

    if (*endptr != '\0') {
        char unit = tolower((unsigned char)*endptr);
        if (unit == 'g') {
            size *= 1024;
        } else if (unit == 't') {
            size *= 1024 * 1024;
        } else if (unit == 'k') {
            size /= 1024;
        }
    }

    return size;
}

/* Simple YAML parser - parses configuration from file */
/* devices:
 *   - memoryLimit: xxx
 *     coresLimit: xxx
 *   - memoryLimit: xxx
 *     coresLimit: xxx
 * globalMemoryLimit: xxx
 * globalCoreLimit: xxx
 * sharedCachePath: xxx
 * oversubscriptionRatio: xxx
 * logging: xxx
 */
static void parse_yaml_config(FILE* fp, hami_core_config_t* config) {
    if (config == NULL || fp == NULL) {
        return;
    }

    char line[MAX_LINE_LEN];
    char key[MAX_VALUE_LEN];
    char value[MAX_VALUE_LEN];
    int current_device_index = -1;
    bool in_devices_section = false;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char line_copy[MAX_LINE_LEN];
        strncpy(line_copy, line, MAX_LINE_LEN - 1);
        line_copy[MAX_LINE_LEN - 1] = '\0';

        char* trimmed = trim_whitespace(line_copy);

        if (strlen(trimmed) == 0 || is_comment(line)) {
            continue;
        }

        int indent = count_indent(line);

        /* Check if this is a YAML array item marker (starts with "- ") */
        bool is_array_item = (trimmed[0] == '-' && (trimmed[1] == ' ' || trimmed[1] == '\t'));
        if (is_array_item) {
            trimmed = trim_whitespace(trimmed + 1);  /* Skip the '-' and whitespace */
        }

        if (extract_key_value(trimmed, key, value)) {
            if (indent == 0) {
                /* Top level keys */
                if (strcasecmp(key, "devices") == 0) {
                    /* Start of devices array */
                    in_devices_section = true;
                    current_device_index = -1;
                } else if (strcasecmp(key, "globalMemoryLimit") == 0) {
                    /* Set global memory limit (in bytes) */
                    config->global_memory_limit = parse_memory_size(value);
                    in_devices_section = false;
                } else if (strcasecmp(key, "globalCoreLimit") == 0) {
                    /* Set global core limit */
                    config->global_core_limit = (size_t)atoi(value);
                    in_devices_section = false;
                } else if (strcasecmp(key, "sharedCachePath") == 0) {
                    strncpy(config->shared_cache_path, value, MAX_STRING_LEN - 1);
                    config->shared_cache_path[MAX_STRING_LEN - 1] = '\0';
                    in_devices_section = false;
                } else if (strcasecmp(key, "oversubscriptionRatio") == 0) {
                    /* Set oversubscription ratio */
                    config->oversubscription_ratio = atof(value);
                    in_devices_section = false;
                }
            } else if (indent >= 2 && in_devices_section) {
                /* Inside devices section - handle device properties */
                if (is_array_item && current_device_index < MAX_DEVICES - 1) {
                    current_device_index++;
                }

                if (current_device_index >= 0 && current_device_index < MAX_DEVICES) {
                    if (strcasecmp(key, "memoryLimit") == 0) {
                        config->devices[current_device_index].memory_limit = parse_memory_size(value);
                    } else if (strcasecmp(key, "coresLimit") == 0) {
                        config->devices[current_device_index].cores_limit = (size_t)atoi(value);
                    }
                }
            }
        }
    }
}


bool hami_load_config_from_file(const char* path, hami_core_config_t* config) {
    if (path == NULL || config == NULL) {
        return false;
    }

    /* Check if file exists and is readable */
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        /* File doesn't exist or is not a regular file */
        return false;
    }

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        return false;
    }

    parse_yaml_config(fp, config);
    fclose(fp);

    return true;
}

void hami_load_config_from_env(hami_core_config_t* config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(hami_core_config_t));

    config->global_memory_limit = get_limit_from_env("CUDA_DEVICE_MEMORY_LIMIT");
    config->global_core_limit = get_limit_from_env("CUDA_DEVICE_SM_LIMIT");

    for (int i = 0; i < MAX_DEVICES; i++) {
        char memory_env_name[64];
        snprintf(memory_env_name, sizeof(memory_env_name), "CUDA_DEVICE_MEMORY_LIMIT_%d", i);
        config->devices[i].memory_limit = get_limit_from_env(memory_env_name);

        char cores_env_name[64];
        snprintf(cores_env_name, sizeof(cores_env_name), "CUDA_DEVICE_SMCORE_LIMIT_%d", i);
        config->devices[i].cores_limit = get_limit_from_env(cores_env_name);
    }

    char* cache_path = getenv(MULTIPROCESS_SHARED_REGION_CACHE_ENV);
    if (cache_path == NULL) {
        cache_path = MULTIPROCESS_SHARED_REGION_CACHE_DEFAULT;
    }
    strncpy(config->shared_cache_path, cache_path, MAX_STRING_LEN - 1);

    const char* oversub = getenv("CUDA_OVERSUBSCRIBE");
    if (oversub != NULL && (strcasecmp(oversub, "true") == 0 || strcmp(oversub, "1") == 0)) {
        config->oversubscription_ratio = hami_get_oversubscription_ratio();
    }

}

void hami_init_default_config(hami_core_config_t* config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(hami_core_config_t));

    strncpy(config->shared_cache_path, MULTIPROCESS_SHARED_REGION_CACHE_DEFAULT, MAX_STRING_LEN - 1);
    config->shared_cache_path[MAX_STRING_LEN - 1] = '\0';
    config->global_core_limit = 100;
}

void hami_load_config(const char* config_path, hami_core_config_t* config) {
    /* Initialize with default values*/
    hami_init_default_config(config);

    /* Try to load from file first */
    if (config_path != NULL && strlen(config_path) > 0) {
        struct stat st;
        if (stat(config_path, &st) == 0 && S_ISREG(st.st_mode)) {
            if (hami_load_config_from_file(config_path, config)) {
                g_config_loaded_from_file = true;
                return ;
            }
        }
    }

    /* Fallback to environment variables */
    hami_load_config_from_env(config);
}

bool hami_config_loaded_from_file(void) {
    return g_config_loaded_from_file;
}

void hami_reset_config_state(void) {
    g_config_loaded_from_file = false;
}

double hami_get_oversubscription_ratio(void) {
    const char* value = getenv("CUDA_OVERSUBSCRIPTION_RATIO");
    if (value != NULL) {
        return atof(value);
    }
    return 1;
}
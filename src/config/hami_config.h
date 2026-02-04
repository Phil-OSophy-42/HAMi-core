/*
 * Copyright (c) 2024, HAMi.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef HAMI_CONFIG_H
#define HAMI_CONFIG_H
#define HAMI_CONFIG_PATH "/hami/config"
#define MULTIPROCESS_SHARED_REGION_CACHE_ENV "CUDA_DEVICE_MEMORY_SHARED_CACHE"
#define MULTIPROCESS_SHARED_REGION_CACHE_DEFAULT "/tmp/cudevshr.cache"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GPU_COUNT 16
#define MAX_STRING_LEN 256
#define MAX_DEVICES 8

typedef struct {
    size_t global_memory_limit;
    size_t global_core_limit;

    struct {
        size_t memory_limit;
        size_t cores_limit;
    } devices[MAX_DEVICES];

    char shared_cache_path[MAX_STRING_LEN];

    double oversubscription_ratio;

} hami_core_config_t;

/* Config loading functions */
void hami_load_config(const char* config_path, hami_core_config_t* config);
bool hami_load_config_from_file(const char* path, hami_core_config_t* config);
void hami_load_config_from_env(hami_core_config_t* config);
bool hami_config_loaded_from_file(void);
void hami_init_default_config(hami_core_config_t* config);
void hami_reset_config_state(void);

double hami_get_oversubscription_ratio(void);

#ifdef __cplusplus
}
#endif

#endif /* HAMI_CONFIG_H */

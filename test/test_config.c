/*
 * Test file for HAMI-core config functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/config/hami_config.h"

/* Mock get_limit_from_env for testing without CUDA dependency */
size_t get_limit_from_env(const char* env_name) {
    char* env_limit = getenv(env_name);
    if (env_limit == NULL) {
        // fprintf(stderr, "No %s set in environment\n", env_name);
        return 0;
    }
    size_t len = strlen(env_limit);
    if (len == 0) {
        // fprintf(stderr, "Empty %s set in environment\n", env_name);
        return 0;
    }
    size_t scalar = 1;
    char* digit_end = env_limit + len;
    if (env_limit[len - 1] == 'G' || env_limit[len - 1] == 'g') {
        digit_end -= 1;
        scalar = 1024 * 1024 * 1024;
    } else if (env_limit[len - 1] == 'M' || env_limit[len - 1] == 'm') {
        digit_end -= 1;
        scalar = 1024 * 1024;
    } else if (env_limit[len - 1] == 'K' || env_limit[len - 1] == 'k') {
        digit_end -= 1;
        scalar = 1024;
    }
    size_t res = strtoul(env_limit, &digit_end, 0);
    size_t scaled_res = res * scalar;
    if (scaled_res == 0) {
        if (env_name[12]=='S'){
        }else if (env_name[12]=='M'){
        }else{
        }
        return 0;
    }
    if (scaled_res != 0 && scaled_res / scalar != res) {
    
        return 0;
    }
    return scaled_res;
}

#define TEST_CONFIG_FILE "/tmp/hami_test_config.yaml"
#define TEST_PASSED 0
#define TEST_FAILED 1

int test_count = 0;
int pass_count = 0;

void test_env_config(void) {
    printf("\n=== Test: Environment Variable Config ===\n");
    test_count++;

    /* Set environment variables */
    setenv("CUDA_DEVICE_MEMORY_LIMIT", "2g", 1);
    setenv("CUDA_DEVICE_SM_LIMIT", "50", 1);
    setenv("CUDA_DEVICE_MEMORY_LIMIT_0", "1g", 1);
    setenv("CUDA_DEVICE_SMCORE_LIMIT_0", "30", 1);
    setenv("CUDA_DEVICE_MEMORY_LIMIT_1", "512m", 1);
    setenv("CUDA_DEVICE_SMCORE_LIMIT_1", "20", 1);

    hami_core_config_t config;
    hami_load_config_from_env(&config);

    /* Verify global limits */
    printf("Global memory limit: %zu (expected: %zu)\n",
           config.global_memory_limit, (size_t)(2UL * 1024 * 1024 * 1024));
    printf("Global core limit: %zu (expected: 50)\n",
           config.global_core_limit);

    assert(config.global_memory_limit == 2UL * 1024 * 1024 * 1024);  /* 2GB in bytes */
    assert(config.global_core_limit == 50);

    /* Verify per-device limits */
    printf("Device 0 memory limit: %zu (expected: %zu)\n",
           config.devices[0].memory_limit, (size_t)(1UL * 1024 * 1024 * 1024));  /* 1GB */
    printf("Device 0 cores limit: %zu (expected: 30)\n",
           config.devices[0].cores_limit);

    assert(config.devices[0].memory_limit == 1UL * 1024 * 1024 * 1024);  /* 1GB in bytes */
    assert(config.devices[0].cores_limit == 30);

    printf("Device 1 memory limit: %zu (expected: %zu)\n",
           config.devices[1].memory_limit, (size_t)(512UL * 1024 * 1024));  /* 512MB */
    printf("Device 1 cores limit: %zu (expected: 20)\n",
           config.devices[1].cores_limit);

    assert(config.devices[1].memory_limit == 512UL * 1024 * 1024);  /* 512MB in bytes */
    assert(config.devices[1].cores_limit == 20);

    printf("TEST PASSED: Environment variable config works correctly!\n");
    pass_count++;

    /* Cleanup */
    unsetenv("CUDA_DEVICE_MEMORY_LIMIT");
    unsetenv("CUDA_DEVICE_SM_LIMIT");
    unsetenv("CUDA_DEVICE_MEMORY_LIMIT_0");
    unsetenv("CUDA_DEVICE_SMCORE_LIMIT_0");
    unsetenv("CUDA_DEVICE_MEMORY_LIMIT_1");
    unsetenv("CUDA_DEVICE_SMCORE_LIMIT_1");
}

void test_yaml_config_file(void) {
    printf("\n=== Test: YAML Config File ===\n");
    test_count++;

    /* Create test YAML config file */
    FILE* fp = fopen(TEST_CONFIG_FILE, "w");
    assert(fp != NULL);

    fprintf(fp, "# Test HAMI-core config file\n");
    fprintf(fp, "globalMemoryLimit: 4g\n");
    fprintf(fp, "globalCoreLimit: 80\n");
    fprintf(fp, "sharedCachePath: /tmp/test_cache\n");
    fprintf(fp, "oversubscriptionRatio: 2.5\n");
    fprintf(fp, "devices:\n");
    fprintf(fp, "  - memoryLimit: 2g\n");
    fprintf(fp, "    coresLimit: 40\n");
    fprintf(fp,  "  - memoryLimit: 1g\n");
    fprintf(fp, "    coresLimit: 20\n");
    fclose(fp);

    /* Load config from file */
    hami_core_config_t config;
    memset(&config, 0, sizeof(config));

    bool result = hami_load_config_from_file(TEST_CONFIG_FILE, &config);
    assert(result == true);

    /* Verify global limits */
    printf("Global memory limit: %zu (expected: %zu)\n",
           config.global_memory_limit, (size_t)(4 * 1024));  /* 4GB */
    printf("Global core limit: %zu (expected: 80)\n",
           config.global_core_limit);
    printf("Shared cache path: %s (expected: /tmp/test_cache)\n",
           config.shared_cache_path);
    printf("Oversubscription ratio: %.1f (expected: 2.5)\n",
           config.oversubscription_ratio);

    assert(config.global_memory_limit == 4 * 1024);  /* 4GB */
    assert(config.global_core_limit == 80);
    assert(strcmp(config.shared_cache_path, "/tmp/test_cache") == 0);
    assert(config.oversubscription_ratio == 2.5);

    /* Verify per-device limits */
    printf("Device 0 memory limit: %zu (expected: %zu)\n",
           config.devices[0].memory_limit, (size_t)(2 * 1024));  /* 2GB */
    printf("Device 0 cores limit: %zu (expected: 40)\n",
           config.devices[0].cores_limit);

    assert(config.devices[0].memory_limit == 2 * 1024);  /* 2GB */
    assert(config.devices[0].cores_limit == 40);

    printf("Device 1 memory limit: %zu (expected: %zu)\n",
           config.devices[1].memory_limit, (size_t)(1 * 1024));  /* 1GB */
    printf("Device 1 cores limit: %zu (expected: 20)\n",
           config.devices[1].cores_limit);

    assert(config.devices[1].memory_limit == 1 * 1024);  /* 1GB */
    assert(config.devices[1].cores_limit == 20);

    printf("TEST PASSED: YAML config file works correctly!\n");
    pass_count++;

    /* Cleanup */
    remove(TEST_CONFIG_FILE);
}

void test_yaml_with_comments_and_empty_lines(void) {
    printf("\n=== Test: YAML with Comments and Empty Lines ===\n");
    test_count++;

    /* Create test YAML config with comments and empty lines */
    FILE* fp = fopen(TEST_CONFIG_FILE, "w");
    assert(fp != NULL);

    fprintf(fp, "# This is a comment\n");
    fprintf(fp, "\n");
    fprintf(fp, "  # Indented comment\n");
    fprintf(fp, "globalMemoryLimit: 3g\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Another comment\n");
    fprintf(fp, "globalCoreLimit: 60\n");
    fclose(fp);

    /* Load config from file */
    hami_core_config_t config;
    memset(&config, 0, sizeof(config));

    bool result = hami_load_config_from_file(TEST_CONFIG_FILE, &config);
    assert(result == true);

    printf("Global memory limit: %zu (expected: %zu)\n",
           config.global_memory_limit, (size_t)(3 * 1024));  /* 3GB */
    printf("Global core limit: %zu (expected: 60)\n",
           config.global_core_limit);

    assert(config.global_memory_limit == 3 * 1024);  /* 3GB */
    assert(config.global_core_limit == 60);

    printf("TEST PASSED: YAML with comments and empty lines works correctly!\n");
    pass_count++;

    /* Cleanup */
    remove(TEST_CONFIG_FILE);
}

void test_nonexistent_file(void) {
    printf("\n=== Test: Non-existent File ===\n");
    test_count++;

    hami_core_config_t config;
    memset(&config, 0, sizeof(config));

    bool result = hami_load_config_from_file("/nonexistent/path/config.yaml", &config);
    assert(result == false);

    printf("Non-existent file load returned: %s (expected: false)\n",
           result ? "true" : "false");

    printf("TEST PASSED: Non-existent file handling works correctly!\n");
    pass_count++;
}

void test_memory_size_parsing(void) {
    printf("\n=== Test: Memory Size Parsing ===\n");
    test_count++;

    /* Create test YAML with different memory size formats */
    FILE* fp = fopen(TEST_CONFIG_FILE, "w");
    assert(fp != NULL);

    fprintf(fp, "globalMemoryLimit: 8g\n");
    fclose(fp);

    hami_core_config_t config;
    memset(&config, 0, sizeof(config));

    bool result = hami_load_config_from_file(TEST_CONFIG_FILE, &config);
    assert(result == true);

    /* 8g should be 8 * 1024 MB = 8192 MB */
    printf("Memory limit for '8g': %zu MB (expected: 8192 MB)\n",
           config.global_memory_limit);
    assert(config.global_memory_limit == 8 * 1024);

    remove(TEST_CONFIG_FILE);

    /* Test with no suffix (raw bytes in MB) */
    fp = fopen(TEST_CONFIG_FILE, "w");
    assert(fp != NULL);
    fprintf(fp, "globalMemoryLimit: 1024\n");
    fclose(fp);

    memset(&config, 0, sizeof(config));
    result = hami_load_config_from_file(TEST_CONFIG_FILE, &config);
    assert(result == true);

    printf("Memory limit for '1024' (no suffix): %zu MB (expected: 1024 MB)\n",
           config.global_memory_limit);
    assert(config.global_memory_limit == 1024);

    printf("TEST PASSED: Memory size parsing works correctly!\n");
    pass_count++;

    remove(TEST_CONFIG_FILE);
}

void test_oversubscription_ratio(void) {
    printf("\n=== Test: Oversubscription Ratio ===\n");
    test_count++;

    /* Test default ratio (env not set) */
    unsetenv("CUDA_OVERSUBSCRIPTION_RATIO");
    double ratio = hami_get_oversubscription_ratio();
    printf("Default oversubscription ratio: %.1f (expected: 1.0)\n", ratio);
    assert(ratio == 1.0);

    /* Test with custom ratio */
    setenv("CUDA_OVERSUBSCRIPTION_RATIO", "3.5", 1);
    ratio = hami_get_oversubscription_ratio();
    printf("Custom oversubscription ratio: %.1f (expected: 3.5)\n", ratio);
    assert(ratio == 3.5);

    printf("TEST PASSED: Oversubscription ratio works correctly!\n");
    pass_count++;

    unsetenv("CUDA_OVERSUBSCRIPTION_RATIO");
}

void test_hami_load_config_priority(void) {
    printf("\n=== Test: Config Loading Priority (File > Env) ===\n");
    test_count++;

    /* Create a config file */
    FILE* fp = fopen(TEST_CONFIG_FILE, "w");
    assert(fp != NULL);
    fprintf(fp, "globalMemoryLimit: 10g\n");
    fprintf(fp, "globalCoreLimit: 100\n");
    fclose(fp);

    /* Set conflicting environment variables */
    setenv("CUDA_DEVICE_MEMORY_LIMIT", "1g", 1);
    setenv("CUDA_DEVICE_SM_LIMIT", "10", 1);

    /* Load config from file path (should prefer file) */
    hami_core_config_t config;
    hami_load_config(TEST_CONFIG_FILE, &config);

    printf("Global memory limit: %zu (expected: %zu from file)\n",
           config.global_memory_limit, (size_t)(10 * 1024));  /* 10GB = 10 * 1024 MB */
    printf("Global core limit: %zu (expected: 100 from file)\n",
           config.global_core_limit);
    printf("Config loaded from file: %s (expected: true)\n",
           hami_config_loaded_from_file() ? "true" : "false");

    assert(config.global_memory_limit == 10 * 1024);  /* From file (MB) */
    assert(config.global_core_limit == 100);           /* From file */
    assert(hami_config_loaded_from_file() == true);

    printf("TEST PASSED: Config loading priority (file > env) works correctly!\n");
    pass_count++;

    /* Cleanup */
    remove(TEST_CONFIG_FILE);
    unsetenv("CUDA_DEVICE_MEMORY_LIMIT");
    unsetenv("CUDA_DEVICE_SM_LIMIT");
}

void test_hami_load_config_env_fallback(void) {
    printf("\n=== Test: Config Env Fallback (No File) ===\n");
    test_count++;

    /* Reset config state from previous tests */
    hami_reset_config_state();

    /* Set environment variables (no file exists) */
    setenv("CUDA_DEVICE_MEMORY_LIMIT", "5g", 1);
    setenv("CUDA_DEVICE_SM_LIMIT", "70", 1);

    /* Load config with non-existent file path */
    hami_core_config_t config;
    hami_load_config("/nonexistent/path/config.yaml", &config);

    printf("Global memory limit: %zu (expected: %zu from env)\n",
           config.global_memory_limit, (size_t)(5UL * 1024 * 1024 * 1024));
    printf("Global core limit: %zu (expected: 70 from env)\n",
           config.global_core_limit);
    printf("Config loaded from file: %s (expected: false)\n",
           hami_config_loaded_from_file() ? "true" : "false");

    assert(config.global_memory_limit == 5UL * 1024 * 1024 * 1024);  /* From env (bytes) */
    assert(config.global_core_limit == 70);                          /* From env */
    assert(hami_config_loaded_from_file() == false);

    printf("TEST PASSED: Config env fallback works correctly!\n");
    pass_count++;

    /* Cleanup */
    unsetenv("CUDA_DEVICE_MEMORY_LIMIT");
    unsetenv("CUDA_DEVICE_SM_LIMIT");
}

void test_default_config(void) {
    printf("\n=== Test: Default Config ===\n");
    test_count++;

    hami_core_config_t config;
    hami_init_default_config(&config);

    printf("Default shared cache path: %s (expected: %s)\n",
           config.shared_cache_path, MULTIPROCESS_SHARED_REGION_CACHE_DEFAULT);
    printf("Default global core limit: %zu (expected: 100)\n",
           config.global_core_limit);

    assert(strcmp(config.shared_cache_path, MULTIPROCESS_SHARED_REGION_CACHE_DEFAULT) == 0);
    assert(config.global_core_limit == 100);

    printf("TEST PASSED: Default config works correctly!\n");
    pass_count++;
}

int main() {
    printf("========================================\n");
    printf("  HAMI-core Config Test Suite\n");
    printf("========================================\n");

    /* Run all tests */
    test_default_config();
    test_env_config();
    test_yaml_config_file();
    test_yaml_with_comments_and_empty_lines();
    test_nonexistent_file();
    test_memory_size_parsing();
    test_oversubscription_ratio();
    test_hami_load_config_priority();
    test_hami_load_config_env_fallback();

    /* Print summary */
    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", test_count - pass_count);
    printf("========================================\n");

    if (pass_count == test_count) {
        printf("All tests PASSED! \\o/\n");
        return TEST_PASSED;
    } else {
        printf("Some tests FAILED! /o\\\n");
        return TEST_FAILED;
    }
}


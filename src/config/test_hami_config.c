/*
 * Test program for hami_config parser
 */

#include "hami_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int run_test(const char* test_name, const char* yaml_content, bool expect_success) {
    printf("\n=== Test: %s ===\n", test_name);

    /* Write YAML to temp file */
    FILE* fp = fopen("/tmp/test_config.yaml", "w");
    if (!fp) {
        printf("ERROR: Cannot create temp file\n");
        return -1;
    }
    fputs(yaml_content, fp);
    fclose(fp);

    /* Parse config */
    hami_core_config_t config;
    memset(&config, 0, sizeof(config));

    bool result = hami_load_config_from_file("/tmp/test_config.yaml", &config);

    /* Print results */
    printf("Result: %s\n", result ? "PASS" : "FAIL");

    if (result) {
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (config.devices[i].memory_limit > 0 || config.devices[i].cores_limit > 0) {
                printf("  Device %d: memory_limit=%lu MB, cores_limit=%lu\n",
                       i, config.devices[i].memory_limit, config.devices[i].cores_limit);
            }
        }

        if (config.global_memory_limit > 0) {
            printf("Global Memory Limit: %lu MB\n", config.global_memory_limit);
        }
        if (config.global_core_limit > 0) {
            printf("Global Core Limit: %lu\n", config.global_core_limit);
        }
        printf("Shared Cache Path: %s\n", config.shared_cache_path);
        printf("Oversubscription Ratio: %.2f\n", config.oversubscription_ratio);
    }

    /* Cleanup */
    unlink("/tmp/test_config.yaml");

    return (result == expect_success) ? 0 : -1;
}

int main(void) {
    int total_tests = 0;
    int passed_tests = 0;

    /* Test 1: Single device with memory and cores */
    total_tests++;
    if (run_test("Single device",
                 "devices:\n"
                 "  - memoryLimit: 8192\n"
                 "    coresLimit: 32\n", true) == 0) {
        passed_tests++;
    }

    /* Test 2: Multiple devices */
    total_tests++;
    if (run_test("Multiple devices",
                 "devices:\n"
                 "  - memoryLimit: 8192\n"
                 "    coresLimit: 32\n"
                 "  - memoryLimit: 16384\n"
                 "    coresLimit: 64\n"
                 "  - memoryLimit: 4096\n"
                 "    coresLimit: 16\n", true) == 0) {
        passed_tests++;
    }

    /* Test 3: Global limits only */
    total_tests++;
    if (run_test("Global limits",
                 "globalMemoryLimit: 32768\n"
                 "globalCoreLimit: 128\n", true) == 0) {
        passed_tests++;
    }

    /* Test 4: Full config with all options */
    total_tests++;
    if (run_test("Full config",
                 "devices:\n"
                 "  - memoryLimit: 8192\n"
                 "    coresLimit: 32\n"
                 "  - memoryLimit: 16384\n"
                 "    coresLimit: 64\n"
                 "globalMemoryLimit: 65536\n"
                 "globalCoreLimit: 256\n"
                 "sharedCachePath: /dev/shm/hami-cache\n"
                 "oversubscriptionRatio: 1.5\n", true) == 0) {
        passed_tests++;
    }

    /* Test 5: Memory size with units */
    total_tests++;
    if (run_test("Memory with units",
                 "devices:\n"
                 "  - memoryLimit: 8g\n"
                 "    coresLimit: 32\n"
                 "  - memoryLimit: 16g\n"
                 "    coresLimit: 64\n", true) == 0) {
        passed_tests++;
    }

    /* Test 6: Empty devices list with global limit */
    total_tests++;
    if (run_test("Empty devices with global",
                 "devices:\n"
                 "globalMemoryLimit: 32768\n", true) == 0) {
        passed_tests++;
    }

    /* Test 7: With comments */
    total_tests++;
    if (run_test("Config with comments",
                 "# This is a comment\n"
                 "devices:\n"
                 "  # First device\n"
                 "  - memoryLimit: 8192\n"
                 "    coresLimit: 32\n"
                 "# End comment\n"
                 "globalMemoryLimit: 32768\n", true) == 0) {
        passed_tests++;
    }

    /* Summary */
    printf("\n=== Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);

    return (passed_tests == total_tests) ? 0 : 1;
}

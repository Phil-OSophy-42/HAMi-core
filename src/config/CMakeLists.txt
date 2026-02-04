cmake_minimum_required(VERSION 3.10)

# Config module for HAMi configuration file parsing

add_library(config_mod OBJECT hami_config.c)
target_include_directories(config_mod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(config_mod PUBLIC ${LIBRARY_COMPILE_FLAGS})

# Test executable for config parser
add_executable(test_hami_config test_hami_config.c hami_config.c)
target_include_directories(test_hami_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.16)

# Standalone SDK package: headers in include/, prebuilt binaries in lib/,
# examples linked against the prebuilt import library.
project(dexhandpro_sdk VERSION 1.0 LANGUAGES CXX)

# The package ships Release binaries only. A Debug executable uses the /MDd
# CRT and a different STL container layout, so passing std::string or
# std::vector across the DLL boundary at runtime is undefined behavior.
get_property(DEXPRO_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(DEXPRO_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Available configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR
        "The SDK package ships Release binaries only. "
        "Configure with -DCMAKE_BUILD_TYPE=Release. "
        "Debug examples would corrupt STL objects across the DLL boundary.")
endif()

add_subdirectory(examples)

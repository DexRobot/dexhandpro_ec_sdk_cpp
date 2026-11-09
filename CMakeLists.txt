cmake_minimum_required(VERSION 3.20)

# Standalone prebuilt SDK package.
# Headers: include/
# Binaries: lib/windows/{Debug,Release}
# Examples: examples/
project(dexhandpro_sdk VERSION 1.0.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "DexHandPro SDK supports Windows only.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DexHandPro SDK requires x64.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR
        "DexHandPro SDK requires the MSVC toolchain (v143 or newer). "
        "MinGW and non-MSVC C++ ABIs are not compatible.")
endif()

if(MSVC_TOOLSET_VERSION LESS 143)
    message(FATAL_ERROR
        "DexHandPro SDK requires MSVC v143 or newer. "
        "The shipped DLLs were built with MSVC v143.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DEXPRO_ROOT       "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEXPRO_INCLUDE   "${DEXPRO_ROOT}/include")
set(DEXPRO_LIB_DIR   "${DEXPRO_ROOT}/lib/windows")

foreach(config Debug Release)
    if(NOT EXISTS "${DEXPRO_LIB_DIR}/${config}/dexhandpro.dll")
        message(FATAL_ERROR "Missing ${config} runtime: ${DEXPRO_LIB_DIR}/${config}/dexhandpro.dll")
    endif()
    if(NOT EXISTS "${DEXPRO_LIB_DIR}/${config}/dexhandpro.lib")
        message(FATAL_ERROR "Missing ${config} import library: ${DEXPRO_LIB_DIR}/${config}/dexhandpro.lib")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Imported target used by examples in this tree.
# ---------------------------------------------------------------------------
if(NOT TARGET DexHandPro::dexhandpro)
    add_library(DexHandPro::dexhandpro SHARED IMPORTED)

    set_target_properties(DexHandPro::dexhandpro PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG   "${DEXPRO_LIB_DIR}/Debug/dexhandpro.dll"
        IMPORTED_IMPLIB_DEBUG     "${DEXPRO_LIB_DIR}/Debug/dexhandpro.lib"
        IMPORTED_LOCATION_RELEASE "${DEXPRO_LIB_DIR}/Release/dexhandpro.dll"
        IMPORTED_IMPLIB_RELEASE   "${DEXPRO_LIB_DIR}/Release/dexhandpro.lib"

        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL     Release

        INTERFACE_INCLUDE_DIRECTORIES "${DEXPRO_INCLUDE}"
        INTERFACE_COMPILE_FEATURES    cxx_std_20
        INTERFACE_COMPILE_DEFINITIONS "NOMINMAX;_CRT_SECURE_NO_WARNINGS")

    set_property(TARGET DexHandPro::dexhandpro PROPERTY IMPORTED_GLOBAL TRUE)
endif()

if(NOT TARGET dexhandpro)
    add_library(dexhandpro ALIAS DexHandPro::dexhandpro)
endif()

# Copy the DLL matching the consumer configuration next to the target.
if(NOT COMMAND dexhandpro_deploy)
    function(dexhandpro_deploy target)
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "dexhandpro_deploy: unknown target '${target}'")
        endif()

        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:DexHandPro::dexhandpro>"
                "$<TARGET_FILE_DIR:${target}>")
    endfunction()
endif()

add_subdirectory(examples)

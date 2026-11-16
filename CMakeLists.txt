cmake_minimum_required(VERSION 3.20)

# Standalone prebuilt SDK package.
# Headers: include/
# Binaries:
#   Windows: libs/windows/dexhandpro.dll + dexhandpro.lib (Release only)
#   Linux:   libs/linux/libdexhandpro.so (Release only)
# Examples: examples/
project(dexhandpro_sdk VERSION 1.0.2 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DexHandPro SDK requires x64.")
endif()

if(WIN32)
    if(NOT MSVC)
        message(FATAL_ERROR
            "DexHandPro SDK on Windows requires the MSVC toolchain (v143 or newer). "
            "MinGW and non-MSVC C++ ABIs are not compatible.")
    endif()

    if(MSVC_TOOLSET_VERSION LESS 143)
        message(FATAL_ERROR
            "DexHandPro SDK requires MSVC v143 or newer. "
            "The shipped DLLs were built with MSVC v143.")
    endif()
else()
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR
            "DexHandPro SDK supports Windows and Linux only; "
            "got ${CMAKE_SYSTEM_NAME}.")
    endif()
    # The Linux library is built with GCC 11 / libstdc++ (GLIBCXX_3.4.29).
    # Use GCC 11+ or Clang with a compatible libstdc++.
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DEXPRO_ROOT    "${CMAKE_CURRENT_SOURCE_DIR}")
set(DEXPRO_INCLUDE "${DEXPRO_ROOT}/include")

if(WIN32)
    set(DEXPRO_LIB_DIR "${DEXPRO_ROOT}/libs/windows")
    set(DEXPRO_RUNTIME "${DEXPRO_LIB_DIR}/dexhandpro.dll")
    set(DEXPRO_IMPLIB  "${DEXPRO_LIB_DIR}/dexhandpro.lib")

    if(NOT EXISTS "${DEXPRO_RUNTIME}")
        message(FATAL_ERROR "Missing release runtime: ${DEXPRO_RUNTIME}")
    endif()
    if(NOT EXISTS "${DEXPRO_IMPLIB}")
        message(FATAL_ERROR "Missing release import library: ${DEXPRO_IMPLIB}")
    endif()
else()
    set(DEXPRO_LIB_DIR "${DEXPRO_ROOT}/libs/linux")
    set(DEXPRO_RUNTIME "${DEXPRO_LIB_DIR}/libdexhandpro.so")

    if(NOT EXISTS "${DEXPRO_RUNTIME}")
        message(FATAL_ERROR "Missing release library: ${DEXPRO_RUNTIME}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Imported target used by examples in this tree.
# ---------------------------------------------------------------------------
if(NOT TARGET DexHandPro::dexhandpro)
    add_library(DexHandPro::dexhandpro SHARED IMPORTED)

    set(_dexpro_interface_defs "DEXHAND_RELEASE_ONLY")
    if(MSVC)
        list(APPEND _dexpro_interface_defs NOMINMAX _CRT_SECURE_NO_WARNINGS)
    endif()

    if(WIN32)
        set_target_properties(DexHandPro::dexhandpro PROPERTIES
            IMPORTED_CONFIGURATIONS "Release"
            IMPORTED_LOCATION_RELEASE "${DEXPRO_RUNTIME}"
            IMPORTED_IMPLIB_RELEASE   "${DEXPRO_IMPLIB}"

            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
            MAP_IMPORTED_CONFIG_MINSIZEREL     Release)
    else()
        set_target_properties(DexHandPro::dexhandpro PROPERTIES
            IMPORTED_LOCATION "${DEXPRO_RUNTIME}")
    endif()

    set_target_properties(DexHandPro::dexhandpro PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DEXPRO_INCLUDE}"
        INTERFACE_COMPILE_FEATURES    cxx_std_20
        INTERFACE_COMPILE_DEFINITIONS "${_dexpro_interface_defs}")

    set_property(TARGET DexHandPro::dexhandpro PROPERTY IMPORTED_GLOBAL TRUE)
endif()

if(NOT TARGET dexhandpro)
    add_library(dexhandpro ALIAS DexHandPro::dexhandpro)
endif()

# Copy the release runtime next to the target so it can be run in place.
if(NOT COMMAND dexhandpro_deploy)
    function(dexhandpro_deploy target)
        if(NOT TARGET "${target}")
            message(FATAL_ERROR "dexhandpro_deploy: unknown target '${target}'")
        endif()

        add_custom_command(TARGET "${target}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:DexHandPro::dexhandpro>"
                "$<TARGET_FILE_DIR:${target}>")

        if(NOT WIN32)
            # The copied .so sits next to the executable.
            set_target_properties("${target}" PROPERTIES BUILD_RPATH "\$ORIGIN")
        endif()
    endfunction()
endif()

add_subdirectory(examples)

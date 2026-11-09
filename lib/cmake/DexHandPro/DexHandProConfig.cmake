# DexHandPro SDK package configuration.
#
# Usage:
#   find_package(DexHandPro CONFIG REQUIRED)
#   target_link_libraries(app PRIVATE DexHandPro::dexhandpro)
#   dexhandpro_deploy(app)
#
# This package is relocatable: it derives the SDK root from its own location.

cmake_minimum_required(VERSION 3.20)

get_filename_component(_DEXPRO_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(DexHandPro_VERSION       "1.0.1")
set(DexHandPro_VERSION_MAJOR "1")
set(DexHandPro_VERSION_MINOR "0")
set(DexHandPro_VERSION_PATCH "1")
set(DexHandPro_INCLUDE_DIR   "${_DEXPRO_PREFIX}/include")
set(DexHandPro_INCLUDE_DIRS  "${DexHandPro_INCLUDE_DIR}")
set(_DEXPRO_LIB_DIR          "${_DEXPRO_PREFIX}/lib/windows")

foreach(_dexpro_config Debug Release)
    if(NOT EXISTS "${_DEXPRO_LIB_DIR}/${_dexpro_config}/dexhandpro.dll")
        message(FATAL_ERROR
            "DexHandPro: missing ${_dexpro_config} runtime in ${_DEXPRO_LIB_DIR}")
    endif()
    if(NOT EXISTS "${_DEXPRO_LIB_DIR}/${_dexpro_config}/dexhandpro.lib")
        message(FATAL_ERROR
            "DexHandPro: missing ${_dexpro_config} import library in ${_DEXPRO_LIB_DIR}")
    endif()
endforeach()

if(NOT TARGET DexHandPro::dexhandpro)
    add_library(DexHandPro::dexhandpro SHARED IMPORTED)

    set_target_properties(DexHandPro::dexhandpro PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release"
        IMPORTED_LOCATION_DEBUG   "${_DEXPRO_LIB_DIR}/Debug/dexhandpro.dll"
        IMPORTED_IMPLIB_DEBUG     "${_DEXPRO_LIB_DIR}/Debug/dexhandpro.lib"
        IMPORTED_LOCATION_RELEASE "${_DEXPRO_LIB_DIR}/Release/dexhandpro.dll"
        IMPORTED_IMPLIB_RELEASE   "${_DEXPRO_LIB_DIR}/Release/dexhandpro.lib"

        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        MAP_IMPORTED_CONFIG_MINSIZEREL     Release

        INTERFACE_INCLUDE_DIRECTORIES "${DexHandPro_INCLUDE_DIR}"
        INTERFACE_COMPILE_FEATURES    cxx_std_20
        INTERFACE_COMPILE_DEFINITIONS "NOMINMAX;_CRT_SECURE_NO_WARNINGS")

    set_property(TARGET DexHandPro::dexhandpro PROPERTY IMPORTED_GLOBAL TRUE)
endif()

if(NOT TARGET dexhandpro)
    add_library(dexhandpro ALIAS DexHandPro::dexhandpro)
endif()

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

set(DexHandPro_LIBRARIES DexHandPro::dexhandpro)
set(DexHandPro_FOUND TRUE)

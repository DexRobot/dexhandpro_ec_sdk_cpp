cmake_minimum_required(VERSION 3.20)

# Can be configured from the package root (add_subdirectory) or directly
# from this directory.
if(NOT PROJECT_NAME)
    project(dexhandpro_examples VERSION 1.0.2 LANGUAGES CXX)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET DexHandPro::dexhandpro)
    # Standalone examples/ configure: the package config ships under
    # libs/cmake, which is not a standard find_package search location.
    set(_DEXPRO_PACKAGE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../libs/cmake/DexHandPro")
    if(NOT DexHandPro_DIR AND EXISTS
            "${_DEXPRO_PACKAGE_DIR}/DexHandProConfig.cmake")
        set(DexHandPro_DIR "${_DEXPRO_PACKAGE_DIR}")
    endif()

    find_package(DexHandPro CONFIG REQUIRED)
endif()

set(DEXPRO_EXAMPLES
    hand_status_demo
    hand_move_demo
    hand_sdo_demo)

foreach(demo IN LISTS DEXPRO_EXAMPLES)
    add_executable(${demo} "${demo}.cpp")

    target_link_libraries(${demo} PRIVATE DexHandPro::dexhandpro)

    if(COMMAND dexhandpro_deploy)
        dexhandpro_deploy(${demo})
    endif()

    set_target_properties(${demo} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    if(MSVC)
        target_compile_options(${demo} PRIVATE /utf-8 /W3 /EHsc /Zc:__cplusplus)
        target_compile_definitions(${demo} PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${demo} PRIVATE -Wall -Wextra)
    endif()
endforeach()

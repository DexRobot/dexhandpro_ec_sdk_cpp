cmake_minimum_required(VERSION 3.16)

# Can be configured from the package root (add_subdirectory) or directly
# from this directory.
if(NOT PROJECT_NAME)
    project(dexhandpro_examples VERSION 1.0 LANGUAGES CXX)
endif()

# Release only: the package ships Release binaries and mixing CRT/STL ABIs
# across the DLL boundary breaks std::string / std::vector at runtime.
get_property(DEXPRO_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(DEXPRO_MULTI_CONFIG)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Available configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR
        "The SDK package ships Release binaries only. "
        "Configure with -DCMAKE_BUILD_TYPE=Release.")
endif()

if(NOT WIN32)
    message(FATAL_ERROR "Only the Windows x64 SDK package is provided for now.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "DexHand 021Pro SDK only supports x64.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DEXPRO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(DEXPRO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/windows")
set(DEXPRO_IMPORT_LIB "${DEXPRO_LIB_DIR}/dexhandpro.lib")
set(DEXPRO_RUNTIME_DLL "${DEXPRO_LIB_DIR}/dexhandpro.dll")

if(NOT EXISTS "${DEXPRO_IMPORT_LIB}")
    message(FATAL_ERROR "Import library not found: ${DEXPRO_IMPORT_LIB}")
endif()

set(DEXPRO_EXAMPLES
    hand_status_demo
    hand_move_demo
    hand_sdo_demo)

foreach(demo IN LISTS DEXPRO_EXAMPLES)
    add_executable(${demo} "${demo}.cpp")

    target_include_directories(${demo} PRIVATE "${DEXPRO_INCLUDE_DIR}")
    target_link_libraries(${demo} PRIVATE "${DEXPRO_IMPORT_LIB}")

    add_custom_command(TARGET ${demo} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DEXPRO_RUNTIME_DLL}" "$<TARGET_FILE_DIR:${demo}>")

    set_target_properties(${demo} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

    if(MSVC)
        target_compile_options(${demo} PRIVATE /utf-8 /W3 /EHsc /Zc:__cplusplus)
        target_compile_definitions(${demo} PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    endif()
endforeach()

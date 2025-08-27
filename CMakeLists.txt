cmake_minimum_required(VERSION 3.16)
project(CodiceCam VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Option to use system packages instead of vcpkg
option(USE_SYSTEM_PACKAGES "Use system packages instead of vcpkg" OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Cross-compilation setup for MinGW-w64
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# Set vcpkg toolchain if not using system packages
if(NOT USE_SYSTEM_PACKAGES AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# Include directories
include_directories(include)

# Add subdirectories
add_subdirectory(third_party)
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Package configuration
set(CPACK_PACKAGE_NAME "CodiceCam")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Webcam-based Codice Marker Detection for TUIO")
set(CPACK_PACKAGE_VENDOR "Codice-Cam Project")

# Windows-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Codice-Cam")
    set(CPACK_NSIS_PACKAGE_NAME "Codice-Cam")
endif()

include(CPack)

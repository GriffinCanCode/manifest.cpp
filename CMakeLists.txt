cmake_minimum_required(VERSION 3.28)

project(Manifest
    VERSION 1.0.0
    DESCRIPTION "Grand Strategy Empire Builder"
    LANGUAGES CXX
)

# Modern C++ standards and features
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Compiler optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-march=native -mtune=native)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4 /WX /arch:AVX2)
endif()

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto")

# Find packages
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Lua REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)

# Core modules
add_subdirectory(src/core)
add_subdirectory(src/render)
add_subdirectory(src/world)
add_subdirectory(src/game)
add_subdirectory(src/ui)
add_subdirectory(src/ai)
add_subdirectory(src/net)
add_subdirectory(src/tools)

# Tests
enable_testing()
add_subdirectory(tests)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    core
    render
    world
    game
    ui
    ai
    net
)

# Assets
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

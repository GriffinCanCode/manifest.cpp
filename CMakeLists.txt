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
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Ensure proper C++23 standard flag is used
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-std=c++23)
endif()

# Modern C++23 compiler optimizations and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Core warnings (GCC)
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Additional GCC-specific warnings
    add_compile_options(
        -Wshadow -Wold-style-cast -Wcast-align -Wunused
        -Woverloaded-virtual -Wconversion -Wsign-conversion
        -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches
        -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion
        -Wformat=2 -Wlifetime -Wimplicit-fallthrough
    )
    
    # Performance optimizations (x86_64 only)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_compile_options(-march=native -mtune=native)
    endif()
    
    # C++23 specific warnings (GCC 12+)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12)
        add_compile_options(-Wc++23-extensions)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Core warnings (Clang - more conservative)
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Clang-supported warnings only
    add_compile_options(
        -Wshadow -Wold-style-cast -Wcast-align -Wunused
        -Woverloaded-virtual -Wconversion -Wsign-conversion
        -Wmisleading-indentation -Wnull-dereference -Wdouble-promotion
        -Wformat=2 -Wimplicit-fallthrough
    )
    
    # Performance optimizations (platform-appropriate)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_compile_options(-march=native -mtune=native)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        add_compile_options(-mcpu=native)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC warnings and optimizations
    add_compile_options(/W4)
    add_compile_options(/permissive- /Zc:__cplusplus /Zc:preprocessor)
    
    # Intel-specific optimizations only on x86_64
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        add_compile_options(/arch:AVX2)
    endif()
endif()

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto")

# Find packages
find_package(Vulkan)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Lua REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PkgConfig REQUIRED)

# Configure Vulkan support
if(Vulkan_FOUND)
    set(VULKAN_AVAILABLE TRUE)
    add_compile_definitions(VULKAN_AVAILABLE=1)
    message(STATUS "Vulkan found: ${Vulkan_VERSION}")
else()
    set(VULKAN_AVAILABLE FALSE)
    message(WARNING "Vulkan SDK not found - Vulkan renderer will be disabled, falling back to OpenGL")
endif()

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

# Tests - temporarily disabled
#enable_testing()
#add_subdirectory(tests)

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
    OpenGL::GL
    glfw
)

# Assets
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# === Code Quality and Formatting Tools ===

# Enable compile commands export for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find clang-format and clang-tidy
find_program(CLANG_FORMAT NAMES clang-format)
find_program(CLANG_TIDY NAMES clang-tidy)

# Create formatting target
if(CLANG_FORMAT)
    # Find all C++ source files
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${CMAKE_SOURCE_DIR}/src/*.cxx
        ${CMAKE_SOURCE_DIR}/src/*.hxx
        ${CMAKE_SOURCE_DIR}/src/*.cc
        ${CMAKE_SOURCE_DIR}/src/*.h
    )
    
    # Format target
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_CXX_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
        VERBATIM
    )
    
    # Check format target (CI-friendly)
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_CXX_SOURCE_FILES}
        COMMENT "Checking code formatting"
        VERBATIM
    )
    
    message(STATUS "clang-format found: ${CLANG_FORMAT}")
    message(STATUS "  Run 'make format' to format code")
    message(STATUS "  Run 'make check-format' to check formatting")
else()
    message(WARNING "clang-format not found - code formatting targets disabled")
endif()

# Create static analysis target
if(CLANG_TIDY)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${ALL_CXX_SOURCE_FILES}
        COMMENT "Running clang-tidy static analysis"
        VERBATIM
    )
    
    message(STATUS "clang-tidy found: ${CLANG_TIDY}")
    message(STATUS "  Run 'make tidy' to run static analysis")
else()
    message(WARNING "clang-tidy not found - static analysis target disabled")
endif()

# Combined code quality target
if(CLANG_FORMAT AND CLANG_TIDY)
    add_custom_target(quality
        DEPENDS check-format tidy
        COMMENT "Running all code quality checks"
    )
    message(STATUS "  Run 'make quality' to run all code quality checks")
endif()

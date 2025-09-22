# Modern C++23 Logging System
cmake_minimum_required(VERSION 3.28)

# Find fmt library for high-performance formatting
find_package(fmt REQUIRED)

# Logging library target
add_library(log INTERFACE)

target_include_directories(log INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(log INTERFACE
    fmt::fmt
)

target_compile_features(log INTERFACE
    cxx_std_23
)

# Add logging system to core library
target_link_libraries(core INTERFACE log)

# Optional: Enable more detailed logging in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(log INTERFACE
        LOG_DEFAULT_LEVEL=0  # Trace level
    )
else()
    target_compile_definitions(log INTERFACE
        LOG_DEFAULT_LEVEL=2  # Info level
    )
endif()

# Thread safety - required for logging system
find_package(Threads REQUIRED)
target_link_libraries(log INTERFACE Threads::Threads)

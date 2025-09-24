include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test utilities library
add_library(test_utils STATIC
    utils/TestUtils.hpp
    utils/TestUtils.cpp
)

target_link_libraries(test_utils 
    PUBLIC 
        gtest_main
        core
        world
        render
        window
)

target_include_directories(test_utils 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Unit tests
add_subdirectory(unit)

# Integration tests  
add_subdirectory(integration)

# Performance tests
add_subdirectory(performance)

# Test discovery
include(GoogleTest)
# gtest_discover_tests will be called in subdirectories after targets are defined

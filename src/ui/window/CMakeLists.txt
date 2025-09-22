# Window Management System CMakeLists.txt

# Find required packages
find_package(glfw3 REQUIRED)

# Core window system library
add_library(window
    Factory.cpp
    Manager.cpp
    Surface.cpp
    backends/GLFW.cpp
    backends/Mock.cpp
)

target_include_directories(window 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(window
    PUBLIC
        core
        glfw
    PRIVATE
        ${CMAKE_DL_LIBS}
)

# Enable Vulkan if available
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        target_compile_definitions(window PUBLIC VULKAN_AVAILABLE=1)
        target_link_libraries(window PUBLIC Vulkan::Vulkan)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    target_link_libraries(window PRIVATE opengl32)
elseif(APPLE)
    target_link_libraries(window PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    target_link_libraries(window PRIVATE GL X11)
endif()

# Compiler-specific options
target_compile_features(window PUBLIC cxx_std_23)

if(MSVC)
    target_compile_options(window PRIVATE /W4)
else()
    target_compile_options(window PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug mode settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(window PRIVATE DEBUG=1)
endif()

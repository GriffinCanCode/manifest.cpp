cmake_minimum_required(VERSION 3.20)
project(CameraTest CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source directory to include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Find required dependencies that the camera system needs
find_package(PkgConfig REQUIRED)

# Core library (simplified version for testing)
add_library(core_minimal INTERFACE)
target_include_directories(core_minimal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# UI library (simplified version for testing)  
add_library(ui_minimal INTERFACE)
target_include_directories(ui_minimal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Camera library
add_library(camera STATIC
    ../src/render/camera/Controls.cpp
    ../src/render/camera/Behavior.cpp
    ../src/render/camera/State.cpp
    ../src/render/camera/Cursor.cpp
    ../src/render/camera/Selection.cpp
    ../src/render/camera/Camera.cpp
)

target_include_directories(camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(camera PRIVATE
    core_minimal
    ui_minimal
)

target_compile_features(camera PRIVATE cxx_std_23)

# Test executable
add_executable(test_camera
    ../src/render/camera/test_camera.cpp
)

target_link_libraries(test_camera PRIVATE camera)

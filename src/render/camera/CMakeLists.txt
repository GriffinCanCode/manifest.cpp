# Camera control system
add_library(camera STATIC
    Controls.cpp
    Behavior.cpp
    State.cpp
    Cursor.cpp
    Selection.cpp
    Camera.cpp
)

target_include_directories(camera PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(camera PRIVATE
    core
    # Note: Circular dependency resolved by moving Camera to core/graphics
    ui
)

# Compiler requirements
target_compile_features(camera PRIVATE cxx_std_23)

# Test executable to verify camera system works
add_executable(test_camera test_camera.cpp)
target_link_libraries(test_camera PRIVATE camera)

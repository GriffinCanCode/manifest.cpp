# UI Module CMakeLists.txt

add_subdirectory(window)

# Create UI library that links window system
add_library(ui INTERFACE)
target_link_libraries(ui INTERFACE window)
target_include_directories(ui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
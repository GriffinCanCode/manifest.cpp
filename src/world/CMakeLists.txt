# Header-only library
add_library(world INTERFACE)

target_include_directories(world 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(world 
    INTERFACE
        core
)

target_compile_features(world 
    INTERFACE 
        cxx_std_23
)

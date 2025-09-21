# Development tools - to be implemented in future phases
add_library(tools INTERFACE)

target_include_directories(tools 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(tools 
    INTERFACE
        core
        render
        world
)

target_compile_features(tools 
    INTERFACE 
        cxx_std_23
)

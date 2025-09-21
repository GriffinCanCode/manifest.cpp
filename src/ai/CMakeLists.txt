# AI systems - to be implemented in future phases
add_library(ai INTERFACE)

target_include_directories(ai 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ai 
    INTERFACE
        core
        world
        game
)

target_compile_features(ai 
    INTERFACE 
        cxx_std_23
)

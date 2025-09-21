# Game systems - to be implemented in future phases
add_library(game INTERFACE)

target_include_directories(game 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(game 
    INTERFACE
        core
        world
)

target_compile_features(game 
    INTERFACE 
        cxx_std_23
)

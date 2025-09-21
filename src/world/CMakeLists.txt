add_library(world STATIC
    tiles/Tile.hpp
    tiles/Map.hpp
    terrain/Generator.hpp
)

target_include_directories(world 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(world 
    PUBLIC
        core
)

target_compile_features(world 
    PUBLIC 
        cxx_std_23
)

# Header-only library, no source files needed
set_target_properties(world PROPERTIES LINKER_LANGUAGE CXX)

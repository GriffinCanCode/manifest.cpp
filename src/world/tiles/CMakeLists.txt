# Tiles module
add_library(tiles STATIC
    Tile.hpp
    Tile.cpp
    Map.hpp
    Province.hpp
    Region.hpp
    Rules.hpp
    Rules.cpp
    Layers.hpp
    Layers.cpp
    Culture.hpp
    Culture.cpp
    Ownership.hpp
    Supply.hpp
    Supply.cpp
)

target_link_libraries(tiles 
    PRIVATE core
    PRIVATE world_generation
)

target_include_directories(tiles
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

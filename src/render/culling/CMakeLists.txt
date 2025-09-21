# Frustum Culling and Spatial Hash System

set(CULLING_SOURCES
    FrustumCuller.cpp
    SpatialHash.cpp
)

set(CULLING_HEADERS
    FrustumCuller.hpp
    SpatialHash.hpp
)

add_library(culling STATIC
    ${CULLING_SOURCES}
    ${CULLING_HEADERS}
)

target_include_directories(culling
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against core math library
target_link_libraries(culling
    PRIVATE
        core_math
)

# Enable modern C++20 features
target_compile_features(culling PRIVATE cxx_std_20)

# Compiler-specific optimizations
if(MSVC)
    target_compile_options(culling PRIVATE /W4 /arch:AVX2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(culling PRIVATE -Wall -Wextra -mavx2 -mfma)
endif()

# Group source files for IDE
source_group("Source Files" FILES ${CULLING_SOURCES})
source_group("Header Files" FILES ${CULLING_HEADERS})

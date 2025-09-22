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
        core
)

# Enable modern C++20 features
target_compile_features(culling PRIVATE cxx_std_20)

# Compiler-specific optimizations
if(MSVC)
    target_compile_options(culling PRIVATE /W4)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        target_compile_options(culling PRIVATE /arch:AVX2)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(culling PRIVATE -Wall -Wextra)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        target_compile_options(culling PRIVATE -mavx2 -mfma)
    endif()
endif()

# Group source files for IDE
source_group("Source Files" FILES ${CULLING_SOURCES})
source_group("Header Files" FILES ${CULLING_HEADERS})

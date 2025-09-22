add_library(generation STATIC
    Noise.hpp
    Plates.hpp
    Rivers.hpp
    Wonders.hpp
)

target_include_directories(generation 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(generation
    PUBLIC
        core
)

target_compile_features(generation 
    PUBLIC 
        cxx_std_23
)

# Enable optimizations for noise generation
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(generation PRIVATE -O3 -ffast-math)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(generation PRIVATE /O2 /fp:fast)
endif()

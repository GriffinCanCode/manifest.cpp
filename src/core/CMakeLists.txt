add_library(core STATIC
    empty.cpp
    types/Types.hpp
    types/Modern.hpp
    memory/Pool.hpp
    math/Vector.hpp
    math/Matrix.hpp
    time/Time.hpp
)

target_include_directories(core 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(core 
    PUBLIC 
        cxx_std_23
)

# Enable SIMD optimizations (platform-specific)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        target_compile_options(core PRIVATE -mavx2 -mfma)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        # ARM64 NEON is enabled by default on Apple Silicon
        target_compile_options(core PRIVATE -O3)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
        target_compile_options(core PRIVATE /arch:AVX2)
    endif()
endif()

# Header-only library, no source files needed
set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)

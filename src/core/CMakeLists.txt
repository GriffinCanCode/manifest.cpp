add_library(core STATIC
    types/Types.hpp
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

# Enable SIMD optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(core PRIVATE -mavx2 -mfma)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(core PRIVATE /arch:AVX2)
endif()

# Header-only library, no source files needed
set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)

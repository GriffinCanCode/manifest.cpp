add_executable(unit_tests
    CoreTypesTest.cpp
    MathTest.cpp
    MemoryTest.cpp
    TimeTest.cpp
    TileTest.cpp
    MapTest.cpp
    TerrainGeneratorTest.cpp
    CameraTest.cpp
    HexRendererTest.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        test_utils
        gtest_main
        core
        world  
        render
)

target_compile_features(unit_tests 
    PRIVATE 
        cxx_std_23
)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(unit_tests PRIVATE /W4)
endif()

# Add to test discovery
include(GoogleTest)
gtest_discover_tests(unit_tests)

add_executable(performance_tests
    MathPerformanceTest.cpp
    WorldPerformanceTest.cpp
)

target_link_libraries(performance_tests
    PRIVATE
        test_utils
        gtest_main
        core
        world
        render
)

target_compile_features(performance_tests 
    PRIVATE 
        cxx_std_23
)

# Enable optimizations for performance tests
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(performance_tests PRIVATE -O3 -DNDEBUG)
endif()

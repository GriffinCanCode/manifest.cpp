add_executable(integration_tests
    WorldGenerationIntegrationTest.cpp
    RenderingIntegrationTest.cpp
    SimpleRenderTest.cpp
    MinimalTriangleTest.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        test_utils
        gtest_main
        core
        world
        render
        window
)

target_compile_features(integration_tests 
    PRIVATE 
        cxx_std_23
)

# Test discovery
gtest_discover_tests(integration_tests)

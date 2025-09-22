cmake_minimum_required(VERSION 3.20)

# Render passes library
add_library(ManifestRenderPasses
    RenderPass.cpp
    RenderPass.hpp
    ShadowPass.cpp
    ShadowPass.hpp
    MainPass.cpp
    MainPass.hpp
    PostProcessPass.cpp
    PostProcessPass.hpp
    RenderGraph.cpp
    RenderGraph.hpp
)

target_include_directories(ManifestRenderPasses
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ManifestRenderPasses
    PUBLIC
        core
        ManifestRenderCommon
    PRIVATE
        # Add any additional dependencies
)

# Enable modern C++ features
target_compile_features(ManifestRenderPasses PUBLIC cxx_std_23)

# Compiler-specific settings
if(MSVC)
    target_compile_options(ManifestRenderPasses PRIVATE /W4 /WX)
else()
    target_compile_options(ManifestRenderPasses PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Set target properties
set_target_properties(ManifestRenderPasses PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Group source files for IDEs
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    RenderPass.cpp RenderPass.hpp
    ShadowPass.cpp ShadowPass.hpp  
    MainPass.cpp MainPass.hpp
    PostProcessPass.cpp PostProcessPass.hpp
    RenderGraph.cpp RenderGraph.hpp
)

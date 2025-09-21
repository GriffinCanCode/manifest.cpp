# Include subdirectories
add_subdirectory(culling)
add_subdirectory(passes)

add_library(render STATIC
    common/Types.hpp
    common/Renderer.hpp
    common/Camera.hpp
    common/HexMesh.hpp
    common/ProceduralHexRenderer.hpp
    common/ProceduralHexRenderer.cpp
    common/HexRenderer.hpp
    common/HexRenderer.cpp
    opengl/OpenGLRenderer.hpp
    opengl/OpenGLRenderer.cpp
    vulkan/VulkanRenderer.hpp  
    vulkan/VulkanRenderer.cpp
    RendererFactory.cpp
)

# Create alias for common render components
add_library(ManifestRenderCommon ALIAS render)

target_include_directories(render 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(render 
    PUBLIC
        core
        culling
        ManifestRenderPasses
        ${Vulkan_LIBRARIES}
        ${OPENGL_LIBRARIES}
        glfw
    PRIVATE
        ${CMAKE_DL_LIBS}
)

target_compile_features(render 
    PUBLIC 
        cxx_std_23
)

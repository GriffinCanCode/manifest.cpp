# Include subdirectories
add_subdirectory(culling)
add_subdirectory(passes)
add_subdirectory(camera)

set(RENDER_SOURCES
    common/Types.hpp
    common/Renderer.hpp
    common/Camera.hpp
    common/HexMesh.hpp
    common/ProceduralHexRenderer.hpp
    common/ProceduralHexRenderer.cpp
    opengl/Core.hpp
    opengl/Core.cpp
    opengl/Converter.hpp
    opengl/Debug.hpp
    opengl/Loader.hpp
    opengl/Resource.hpp
    opengl/OpenGL.hpp
    opengl/OpenGLRenderer.cpp
    RendererFactory.cpp
)

add_library(render STATIC ${RENDER_SOURCES})

# Create alias for common render components
add_library(ManifestRenderCommon ALIAS render)

target_include_directories(render 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(render 
    PUBLIC
        core
        culling
        ManifestRenderPasses
        camera
        ${OPENGL_LIBRARIES}
        glfw
    PRIVATE
        ${CMAKE_DL_LIBS}
)

# Note: Vulkan libraries removed - using OpenGL only

target_compile_features(render 
    PUBLIC 
        cxx_std_23
)

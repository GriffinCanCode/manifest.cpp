# Include subdirectories
add_subdirectory(culling)
add_subdirectory(passes)

set(RENDER_SOURCES
    common/Types.hpp
    common/Renderer.hpp
    common/Camera.hpp
    common/HexMesh.hpp
    common/ProceduralHexRenderer.hpp
    common/ProceduralHexRenderer.cpp
    common/HexRenderer.hpp
    common/HexRenderer.cpp
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

# Add Vulkan sources only if available
if(VULKAN_AVAILABLE)
    list(APPEND RENDER_SOURCES
        vulkan/VulkanRenderer.hpp  
        vulkan/VulkanRenderer.cpp
        vulkan/VkCore.hpp
        vulkan/VkCore.cpp
        vulkan/VkDevice.hpp
        vulkan/VkDevice.cpp
        vulkan/VkMemory.hpp
        vulkan/VkMemory.cpp
        vulkan/VkResources.hpp
        vulkan/VkResources.cpp
        vulkan/VkCommands.hpp
        vulkan/VkCommands.cpp
        vulkan/VkUtils.hpp
        vulkan/VkUtils.cpp
    )
endif()

add_library(render STATIC ${RENDER_SOURCES})

# Create alias for common render components
add_library(ManifestRenderCommon ALIAS render)

target_include_directories(render 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add Vulkan includes only if available
if(VULKAN_AVAILABLE)
    target_include_directories(render PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

target_link_libraries(render 
    PUBLIC
        core
        culling
        ManifestRenderPasses
        ${OPENGL_LIBRARIES}
        glfw
    PRIVATE
        ${CMAKE_DL_LIBS}
)

# Add Vulkan libraries only if available
if(VULKAN_AVAILABLE)
    target_link_libraries(render PUBLIC ${Vulkan_LIBRARIES})
endif()

target_compile_features(render 
    PUBLIC 
        cxx_std_23
)

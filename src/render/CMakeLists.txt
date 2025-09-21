add_library(render STATIC
    common/Types.hpp
    common/Renderer.hpp
    common/Camera.hpp
    opengl/OpenGLRenderer.hpp
    opengl/OpenGLRenderer.cpp
    vulkan/VulkanRenderer.hpp  
    vulkan/VulkanRenderer.cpp
    RendererFactory.cpp
)

target_include_directories(render 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(render 
    PUBLIC
        core
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

#pragma once

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <string_view>

// Define GL constants if not available (for debug labeling)
#ifndef GL_BUFFER
#define GL_BUFFER 0x82E0
#endif

#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#endif

#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#endif

#ifndef GL_DEBUG_SOURCE_APPLICATION
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#endif

#ifndef GL_DEBUG_TYPE_MARKER
#define GL_DEBUG_TYPE_MARKER 0x8268
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#endif

namespace Manifest {
namespace Render {
namespace OpenGL {

class Debug {
   public:
    static void APIENTRY callback([[maybe_unused]] GLenum source, [[maybe_unused]] GLenum type, [[maybe_unused]] GLuint id, GLenum severity,
                                  [[maybe_unused]] GLsizei length, [[maybe_unused]] const GLchar* message, [[maybe_unused]] const void* user_param) {
        // Handle OpenGL debug messages
        if (severity == GL_DEBUG_SEVERITY_HIGH) {
            // Log error
        } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
            // Log warning
        }
    }

    static void set_object_label([[maybe_unused]] GLenum identifier, [[maybe_unused]] GLuint name, [[maybe_unused]] std::string_view label) {
#ifdef GL_KHR_debug
        if (glObjectLabel) {
            glObjectLabel(identifier, name, static_cast<GLsizei>(label.length()), label.data());
        }
#endif
    }

    static void push_group([[maybe_unused]] std::string_view name) {
#ifdef GL_KHR_debug
        if (glPushDebugGroup) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(name.length()), name.data());
        }
#endif
    }

    static void pop_group() {
#ifdef GL_KHR_debug
        if (glPopDebugGroup) {
            glPopDebugGroup();
        }
#endif
    }

    static void insert_marker([[maybe_unused]] std::string_view name) {
#ifdef GL_KHR_debug
        if (glDebugMessageInsert) {
            glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
                                GL_DEBUG_SEVERITY_NOTIFICATION, static_cast<GLsizei>(name.length()), name.data());
        }
#endif
    }

    static void enable_debug_output() {
#ifdef GL_DEBUG_OUTPUT
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(callback, nullptr);
        }
#endif
    }
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest

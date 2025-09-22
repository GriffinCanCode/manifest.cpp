#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "../common/Types.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

class Loader {
    std::unordered_map<ShaderHandle, std::string> shader_paths_;

   public:
    void register_shader(ShaderHandle handle, const std::string& path) {
        shader_paths_[handle] = path;
    }

    void unregister_shader(ShaderHandle handle) {
        shader_paths_.erase(handle);
    }

    std::vector<ShaderHandle> check_for_changes() {
        // Stub implementation - no hot reloading for now
        return {};
    }
};

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
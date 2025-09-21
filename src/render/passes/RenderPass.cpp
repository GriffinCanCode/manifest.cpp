#include "RenderPass.hpp"
#include "ShadowPass.hpp"
#include "MainPass.hpp" 
#include "PostProcessPass.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

std::unique_ptr<RenderPass> create_render_pass(PassType type) {
    switch (type) {
        case PassType::Shadow:
            return std::make_unique<ShadowPass>();
        case PassType::Main:
            return std::make_unique<MainPass>();
        case PassType::PostProcess:
            return std::make_unique<PostProcessPass>();
        default:
            return nullptr;
    }
}

} // namespace Passes
} // namespace Render
} // namespace Manifest

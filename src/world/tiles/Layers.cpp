#include "Layers.hpp"

namespace Manifest::World::Tiles {

LayerRegistry::LayerRegistry() {
    // Define standard layer transitions
    
    // Surface to Underground (requires excavation)
    add_transition(LayerTransition{Layer::Surface, Layer::Underground, 2.0F});
    
    // Underground to Surface (easier going up)
    add_transition(LayerTransition{Layer::Underground, Layer::Surface, 1.0F});
    
    // Surface to Air (requires flight capability)
    add_transition(LayerTransition{Layer::Surface, Layer::Air, 5.0F});
    
    // Air to Surface (landing)
    add_transition(LayerTransition{Layer::Air, Layer::Surface, 1.0F});
    
    // Air to Space (requires advanced tech)
    add_transition(LayerTransition{Layer::Air, Layer::Space, 10.0F});
    
    // Space to Air (re-entry)
    add_transition(LayerTransition{Layer::Space, Layer::Air, 3.0F});
}

LayerRegistry& get_layer_registry() {
    static LayerRegistry registry;
    return registry;
}

} // namespace Manifest::World::Tiles

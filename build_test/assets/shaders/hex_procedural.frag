#version 460 core

// Input from vertex shader
in VertexData {
    vec3 world_position;
    vec3 world_normal;
    vec2 local_uv;
    vec4 color;
    float elevation;
    flat uint terrain_type;
} vertex_in;

// Uniform buffer for lighting
layout(std140, binding = 1) uniform LightingData {
    vec3 sun_direction;
    vec3 sun_color;
    vec3 ambient_color;
    float sun_intensity;
    vec3 camera_position;
    float _padding;
};

// Output color
out vec4 fragment_color;

// Terrain type constants (should match C++ enum)
const uint TERRAIN_OCEAN = 0u;
const uint TERRAIN_GRASSLAND = 1u;
const uint TERRAIN_FOREST = 2u;
const uint TERRAIN_DESERT = 3u;
const uint TERRAIN_MOUNTAIN = 4u;
const uint TERRAIN_TUNDRA = 5u;

// Procedural terrain coloring
vec3 get_terrain_base_color(uint terrain_type, float elevation) {
    switch (terrain_type) {
        case TERRAIN_OCEAN:
            return mix(vec3(0.1, 0.2, 0.6), vec3(0.0, 0.1, 0.4), elevation);
        case TERRAIN_GRASSLAND:
            return mix(vec3(0.3, 0.7, 0.2), vec3(0.2, 0.5, 0.1), elevation);
        case TERRAIN_FOREST:
            return mix(vec3(0.1, 0.4, 0.1), vec3(0.05, 0.3, 0.05), elevation);
        case TERRAIN_DESERT:
            return mix(vec3(0.8, 0.7, 0.3), vec3(0.7, 0.6, 0.2), elevation);
        case TERRAIN_MOUNTAIN:
            return mix(vec3(0.5, 0.4, 0.3), vec3(0.7, 0.7, 0.7), elevation);
        case TERRAIN_TUNDRA:
            return mix(vec3(0.8, 0.8, 0.9), vec3(0.9, 0.9, 1.0), elevation);
        default:
            return vec3(0.5, 0.5, 0.5); // Gray fallback
    }
}

// Simple noise function for texture variation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
    // Get base terrain color
    vec3 base_color = get_terrain_base_color(vertex_in.terrain_type, vertex_in.elevation);
    
    // Add noise variation for texture
    float texture_noise = noise(vertex_in.world_position.xz * 8.0) * 0.15;
    base_color *= (1.0 + texture_noise);
    
    // Calculate lighting
    vec3 normal = normalize(vertex_in.world_normal);
    vec3 light_dir = normalize(-sun_direction);
    
    // Diffuse lighting
    float diffuse = max(dot(normal, light_dir), 0.0);
    
    // Simple specular for water
    float specular = 0.0;
    if (vertex_in.terrain_type == TERRAIN_OCEAN) {
        vec3 view_dir = normalize(camera_position - vertex_in.world_position);
        vec3 reflect_dir = reflect(-light_dir, normal);
        specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0) * 0.5;
    }
    
    // Combine lighting
    vec3 ambient = ambient_color * 0.3;
    vec3 diffuse_color = sun_color * sun_intensity * diffuse;
    vec3 specular_color = sun_color * specular;
    
    vec3 final_color = base_color * (ambient + diffuse_color) + specular_color;
    
    // Add elevation-based darkening for depth
    final_color *= mix(0.7, 1.0, vertex_in.elevation);
    
    // Edge darkening for hex borders (procedural wireframe effect)
    float edge_distance = min(min(vertex_in.local_uv.x, 1.0 - vertex_in.local_uv.x),
                             min(vertex_in.local_uv.y, 1.0 - vertex_in.local_uv.y));
    float edge_factor = smoothstep(0.0, 0.05, edge_distance);
    final_color *= mix(0.8, 1.0, edge_factor);
    
    fragment_color = vec4(final_color, 1.0);
}

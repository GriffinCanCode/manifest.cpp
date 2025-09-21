#version 460 core

// Vertex attributes (minimal - only instance data)
layout(location = 0) in vec3 instance_position;    // World position of hex center
layout(location = 1) in vec4 instance_color;       // Terrain color
layout(location = 2) in float instance_elevation;  // Elevation (0.0 - 1.0)
layout(location = 3) in uint instance_terrain;     // Terrain type

// Uniform buffer for global rendering data
layout(std140, binding = 0) uniform GlobalData {
    mat4 view_projection_matrix;
    vec3 camera_position;
    float hex_radius;
    float height_scale;
    float time;
    vec2 _padding;
};

// Output to fragment shader
out VertexData {
    vec3 world_position;
    vec3 world_normal;
    vec2 local_uv;
    vec4 color;
    float elevation;
    flat uint terrain_type;
} vertex_out;

// Constants for hex generation
const int HEX_SIDES = 6;
const int VERTICES_PER_HEX = 7; // Center + 6 vertices
const float PI = 3.14159265359;
const float HEX_ANGLE_STEP = (2.0 * PI) / float(HEX_SIDES);

// Generate hex vertex positions procedurally
vec2 get_hex_vertex_offset(int vertex_id) {
    if (vertex_id == 0) {
        // Center vertex
        return vec2(0.0, 0.0);
    }
    
    // Edge vertices (1-6)
    int edge_index = vertex_id - 1;
    float angle = float(edge_index) * HEX_ANGLE_STEP;
    return vec2(cos(angle), sin(angle)) * hex_radius;
}

// Get UV coordinates for hex vertex
vec2 get_hex_vertex_uv(int vertex_id) {
    if (vertex_id == 0) {
        return vec2(0.5, 0.5); // Center
    }
    
    int edge_index = vertex_id - 1;
    float angle = float(edge_index) * HEX_ANGLE_STEP;
    return vec2(0.5 + 0.5 * cos(angle), 0.5 + 0.5 * sin(angle));
}

// Calculate smooth normal based on neighboring elevations (simplified)
vec3 calculate_hex_normal(vec2 hex_offset, float elevation) {
    // For now, use simple normal calculation
    // In a full implementation, you'd sample neighboring tiles
    vec3 normal = vec3(0.0, 1.0, 0.0);
    
    // Add some variation based on position for more interesting lighting
    float noise = sin(hex_offset.x * 10.0) * cos(hex_offset.y * 10.0) * 0.1;
    normal.x += noise;
    normal.z += noise * 0.5;
    
    return normalize(normal);
}

void main() {
    // Get vertex ID within the hex (0-6)
    int vertex_id = gl_VertexID % VERTICES_PER_HEX;
    
    // Generate hex vertex offset
    vec2 hex_offset = get_hex_vertex_offset(vertex_id);
    
    // Calculate world position
    vec3 world_pos = instance_position + vec3(hex_offset.x, 
                                             instance_elevation * height_scale, 
                                             hex_offset.y);
    
    // Calculate normal
    vec3 world_normal = calculate_hex_normal(hex_offset, instance_elevation);
    
    // Get UV coordinates
    vec2 local_uv = get_hex_vertex_uv(vertex_id);
    
    // Transform to clip space
    gl_Position = view_projection_matrix * vec4(world_pos, 1.0);
    
    // Output vertex data
    vertex_out.world_position = world_pos;
    vertex_out.world_normal = world_normal;
    vertex_out.local_uv = local_uv;
    vertex_out.color = instance_color;
    vertex_out.elevation = instance_elevation;
    vertex_out.terrain_type = instance_terrain;
}

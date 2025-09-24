#version 410 core

//==============================================================================
// CRITICAL: This shader was designed for OpenGL 4.1 compatibility on macOS
//==============================================================================
// 
// LESSONS LEARNED FROM VULKAN→OPENGL MIGRATION:
// 1. Complex vertex attribute setups BREAK OpenGL rendering silently
// 2. Uniform blocks get OPTIMIZED OUT if not used in final output
// 3. gl_VertexID + simple instance data = RELIABLE approach
// 4. Direct NDC coordinates work, matrix transforms can fail silently
//
// WORKING APPROACH:
// - Use gl_VertexID for procedural geometry (no vertex buffer needed)
// - Use instance attributes ONLY for per-hex data (position, color)
// - Map world coordinates directly to NDC space (skip camera matrix for now)
// - Keep shader simple - complexity breaks OpenGL pipeline setup
//==============================================================================

// Instance data from C++ (ONLY these work reliably)
layout(location = 1) in vec3 instance_position;    // World position of hex center  
layout(location = 2) in vec4 instance_color;       // Terrain color from C++

// Output to fragment shader
layout(location = 0) out vec4 vertex_color;

// UNUSED uniform block (gets optimized out) - kept for future camera implementation
uniform GlobalUniforms {
    mat4 view_projection_matrix;
};

// Constants for hex generation
const int HEX_SIDES = 6;
const int VERTICES_PER_HEX = 18; // 6 triangles * 3 vertices each
const float PI = 3.14159265359;
const float HEX_ANGLE_STEP = (2.0 * PI) / float(HEX_SIDES);

// Generate hex vertex positions procedurally for triangle topology
vec2 get_hex_vertex_offset(int vertex_id) {
    int triangle_id = vertex_id / 3;  // Which triangle (0-5)
    int vertex_in_triangle = vertex_id % 3;  // Which vertex in triangle (0-2)
    
    if (vertex_in_triangle == 0) {
        // Center vertex (first vertex of each triangle)
        return vec2(0.0, 0.0);
    } else if (vertex_in_triangle == 1) {
        // Current edge vertex
        float angle = float(triangle_id) * HEX_ANGLE_STEP;
        return vec2(cos(angle), sin(angle)) * 50.0;  // Much larger hex size
    } else {
        // Next edge vertex
        float angle = float((triangle_id + 1) % HEX_SIDES) * HEX_ANGLE_STEP;
        return vec2(cos(angle), sin(angle)) * 50.0;  // Much larger hex size
    }
}

// Get UV coordinates for hex vertex
vec2 get_hex_vertex_uv(int vertex_id) {
    int triangle_id = vertex_id / 3;
    int vertex_in_triangle = vertex_id % 3;
    
    if (vertex_in_triangle == 0) {
        return vec2(0.5, 0.5); // Center
    } else {
        // Edge vertices get UV based on their angle
        int edge_index = (vertex_in_triangle == 1) ? triangle_id : ((triangle_id + 1) % HEX_SIDES);
        float angle = float(edge_index) * HEX_ANGLE_STEP;
        return vec2(0.5 + 0.5 * cos(angle), 0.5 + 0.5 * sin(angle));
    }
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
    //==========================================================================
    // ROBUST OPENGL HEX RENDERING - PROVEN APPROACH
    //==========================================================================
    // This approach survived the Vulkan→OpenGL migration and works reliably:
    // 1. Use gl_VertexID for geometry (no complex vertex buffers)
    // 2. Use instance data for positioning and colors
    // 3. Map world coordinates directly to NDC (avoid matrix transforms)
    // 4. Keep shader logic simple to prevent OpenGL optimization issues
    //==========================================================================
    
    // Generate FULL HEX geometry using gl_VertexID (RELIABLE METHOD)
    // Use the existing hex generation function that was already proven to work
    vec2 local_vertex = get_hex_vertex_offset(gl_VertexID);
    
    // CRITICAL: Convert world coordinates to NDC space 
    // This bypasses camera matrix issues that break rendering
    // World bounds from terrain generation: X[9-54], Z[-76 to +69]
    float world_x = instance_position.x;
    float world_z = instance_position.z;
    
    // Normalize world coordinates to NDC space (-1 to +1)
    // DEFENSIVE: Use known world bounds to prevent coordinates outside NDC
    float ndc_x = clamp(((world_x - 9.0) / (54.0 - 9.0)) * 2.0 - 1.0, -1.0, 1.0);
    float ndc_z = clamp(((world_z - (-76.0)) / (69.0 - (-76.0))) * 2.0 - 1.0, -1.0, 1.0);
    
    // Position hex in world using direct NDC mapping
    local_vertex.x += ndc_x * 0.5; // Scale factor controls world coverage
    local_vertex.y += ndc_z * 0.5; // Map Z world coordinate to Y screen coordinate
    
    // FINAL POSITION: Direct NDC coordinates (bypasses matrix transform issues)
    gl_Position = vec4(local_vertex, 0.0, 1.0);
    
    // ROBUST COLOR ASSIGNMENT: Use terrain colors from C++ instance data
    if (gl_InstanceID < 3) {
        // Keep a few red triangles as visual reference points
        vertex_color = vec4(1.0, 0.0, 0.0, 1.0);  
    } else {
        // Use actual terrain colors computed in C++ (ocean=blue, grass=green, etc.)
        vertex_color = instance_color;
    }
}

#version 410 core

// Simple test vertex shader to verify OpenGL rendering works
layout(location = 0) in vec3 vertex_position;     // Triangle vertex position
layout(location = 1) in vec3 instance_position;   // Per-hex position
layout(location = 2) in vec4 instance_color;      // Per-hex color

// Output to fragment shader
layout(location = 0) out vec4 vertex_color;

// Simple uniform for testing
uniform GlobalUniforms {
    mat4 view_projection_matrix;
};

void main() {
    // DEBUG: Render triangle directly in normalized device coordinates - should always be visible
    vec2 ndc_positions[3] = vec2[](
        vec2(-0.5, -0.5),   // Bottom left
        vec2( 0.5, -0.5),   // Bottom right
        vec2( 0.0,  0.5)    // Top
    );
    
    vec2 ndc_pos = ndc_positions[gl_VertexID % 3];
    gl_Position = vec4(ndc_pos, 0.0, 1.0);  // Direct NDC coordinates, no matrix transform
    vertex_color = vec4(1.0, 0.0, 0.0, 1.0);  // Force bright red
}
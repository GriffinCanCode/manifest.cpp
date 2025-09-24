#version 410 core

// Simplified fragment shader for OpenGL compatibility

// Input from vertex shader  
layout(location = 0) in vec4 vertex_color;

// Output color
layout(location = 0) out vec4 fragment_color;

void main() {
    // Use vertex color directly - should show terrain colors if instance data flows
    fragment_color = vertex_color;
}
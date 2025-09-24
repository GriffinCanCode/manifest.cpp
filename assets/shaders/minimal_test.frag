#version 410 core

// Simple test fragment shader
layout(location = 0) in vec4 vertex_color;
layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = vertex_color;
}
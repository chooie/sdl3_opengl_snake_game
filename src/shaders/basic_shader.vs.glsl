#version 330 core

// Input vertex attributes (from vertex buffer)
layout (location = 0) in vec2 aPos;  // Position: x, y

uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
#version 330 core

layout(location = 0) in vec3 aPos;       // Vertex position
layout(location = 1) in vec2 aTexCoord; // Texture coordinates

uniform mat4 MVP;

out vec2 TexCoord; // Pass texture coordinates to fragment shader

void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
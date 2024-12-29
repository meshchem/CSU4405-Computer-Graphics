#version 330 core

layout(location = 0) in vec3 vertexPosition; // Position of each vertex
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV; // TODO: To add UV to this vertex shader 

uniform mat4 MVP; // Model-View-Projection matrix

out vec2 UV; // Pass UV coordinates to fragment shader

void main() {
    gl_Position = MVP * vec4(vertexPosition, 1.0);
    UV = vertexUV;
}


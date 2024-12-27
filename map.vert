#version 330 core

layout(location = 0) in vec3 vertexPosition;        // Vertex position
layout(location = 1) in vec2 vertexUV;              // Texture coordinates
layout(location = 2) in vec3 vertexNormal;          // Normal coordinates

uniform mat4 MVP;               // Model-View-Projection matrix
uniform mat4 model;             // Model matrix
uniform mat4 lightSpaceMatrix;  // Light-space transformation matrix

out vec2 uv;                   // Pass UV coordinates to fragment shader
out vec3 FragPos;              // Pass world position to fragment shader
out vec3 Normal;               // Pass transformed normals to fragment shader
out vec4 FragPosLightSpace;    // Pass light-space position to fragment shader

void main() {
    // Transform vertex position to clip space
    gl_Position = MVP * vec4(vertexPosition, 1.0);

    // Calculate world position
    FragPos = vec3(model * vec4(vertexPosition, 1.0)); 

    // Calculate light-space position
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // Transform normals
    Normal = vertexNormal;

    // Pass UV coordinates
    uv = vertexUV;
}

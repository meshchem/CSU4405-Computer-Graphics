#version 330 core

layout(location = 0) in vec3 vertexPosition;        // Vertex position
layout(location = 1) in vec2 vertexUV;              // Texture coordinates
layout(location = 2) in vec3 vertexNormal;          // Normal coordinates

uniform mat4 MVP;
uniform mat4 model; 

out vec2 uv;          // Pass UV coordinates to fragment shader
out vec3 FragPos;     // Pass world position to fragment shader
out vec3 Normal;      // Pass transformed normals to fragment shader

void main() {
    gl_Position = MVP * vec4(vertexPosition, 1.0);
    FragPos = vec3(model * vec4(vertexPosition, 1.0)); // Calculate world position
    Normal = mat3(transpose(inverse(model))) * vertexNormal; // Transform normals
   // Normal = vertexNormal;
    uv = vertexUV; // Pass UV coordinates
}

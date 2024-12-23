#version 330 core

layout(location = 0) in vec3 position;   // Vertex position
layout(location = 1) in vec3 color;      // Vertex color
layout(location = 2) in vec2 texCoord;   // Texture coordinate
layout(location = 3) in vec3 normal;     // Vertex normal

out vec3 FragPos;       // Fragment position in world space
out vec3 Normal;        // Normal in world space
out vec3 VertexColor;   // Color passed to fragment shader
out vec2 TexCoord;      // Texture coordinate passed to fragment shader

uniform mat4 MVP;       // Combined Model-View-Projection matrix
uniform mat4 model;     // Model matrix

void main() {
    // Transform vertex position
    vec4 worldPosition = model * vec4(position, 1.0);
    FragPos = vec3(worldPosition);

    // Transform normals (accounting for non-uniform scaling)
    Normal = mat3(transpose(inverse(model))) * normal;

    // Pass attributes to fragment shader
    VertexColor = color;
    TexCoord = texCoord;

    // Compute final position
    gl_Position = MVP * vec4(position, 1.0);
}

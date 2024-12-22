#version 330 core

layout(location = 0) in vec3 aPos;       // Vertex position
layout(location = 1) in vec2 aTexCoord;  // Texture coordinates

uniform mat4 MVP;        // Combined Model-View-Projection matrix
uniform mat4 model;      // Model matrix
uniform mat3 normalMatrix; // Normal matrix for transforming normals

out vec2 TexCoord;       // Pass texture coordinates to fragment shader
out vec3 FragPos;        // Pass fragment position in world space
out vec3 Normal;         // Pass transformed normal to fragment shader

void main() {
    // Transform the vertex position to clip space
    gl_Position = MVP * vec4(aPos, 1.0);

    // Pass the fragment position in world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Calculate and pass the normal vector
    Normal = normalize(normalMatrix * vec3(0.0, 1.0, 0.0)); // Assuming flat tiles with upward normals

    // Pass texture coordinates
    TexCoord = aTexCoord;
}

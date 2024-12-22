#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;  // Vertex position
layout(location = 1) in vec3 vertexColor;     // Vertex color (not used for lighting)
layout(location = 2) in vec2 vertexUV;        // Vertex texture coordinates
layout(location = 3) in vec3 vertexNormal;    // Vertex normal for lighting calculations

// Output data, to be interpolated for each fragment
out vec2 uv;            // Pass texture coordinates to fragment shader
out vec3 fragPos;       // Pass fragment position to fragment shader
out vec3 normal;        // Pass transformed normal to fragment shader

// Matrix for vertex transformation
uniform mat4 MVP;       // Combined Model-View-Projection matrix
uniform mat4 model;     // Model matrix
uniform mat3 normalMatrix; // Normal matrix for transforming normals

void main() {
    // Transform vertex position to clip space
    gl_Position = MVP * vec4(vertexPosition, 1.0);

    // Pass the fragment position in world space
    fragPos = vec3(model * vec4(vertexPosition, 1.0));

    // Transform the normal using the normal matrix
    normal = normalize(normalMatrix * vertexNormal);

    // Pass texture coordinates
    uv = vertexUV;
}





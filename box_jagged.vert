#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec2 vertexUV;

// Output data, to be interpolated for each fragment
out vec3 color;
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 uv;

// Matrix for vertex transformation
uniform mat4 MVP;

uniform mat4 model;         // Model matrix for each building
//uniform mat4 view;          // View matrix
//uniform mat4 projection;    // Projection matrix


void main() {
    
    // Pass vertex color to the fragment shader
    color = vertexColor;

    // Pass UV to the fragment shader    
    uv = vertexUV;

     // Transform vertex position
    worldPosition = vec3(model * vec4(vertexPosition, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    // Transform normals (accounting for non-uniform scaling)
    worldNormal = normalize(normalMatrix * vertexNormal);

    // Compute final position
    gl_Position =  MVP * vec4(vertexPosition, 1);
}

#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec3 vertexNormal;

out vec2 uv; 
out vec3 color;
out vec3 Normal; 
out vec3 FragPos; 
out vec4 FragPosLightSpace; // Position in light space

// Matrix for vertex transformation
uniform mat4 model; 
uniform mat4 MVP;
uniform mat4 lightSpaceMatrix; // Light-space transformation matrix

void main() {
    // Transform the vertex position
    gl_Position = MVP * vec4(vertexPosition, 1);
    FragPos = vec3(model * vec4(vertexPosition, 1.0));

    // Transform fragment position to light space
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // Pass UV coordinates and color to the fragment shader
    uv = vertexUV; 
    color = vertexColor; 
    
    // Transform the normal to world space and pass it to the fragment shader
    Normal = vertexNormal;
}

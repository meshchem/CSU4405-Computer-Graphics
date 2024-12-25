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

// Matrix for vertex transformation
uniform mat4 model; 
uniform mat4 MVP;


void main() {
    	
	// Transform the vertex position
    gl_Position = MVP * vec4(vertexPosition, 1);
    FragPos = vec3(model * vec4(vertexPosition, 1.0));

    // Pass UV coordinates and color to the fragment shader
    uv = vertexUV; 
    color = vertexColor; 
    
    // Transform the normal to world space and pass it to the fragment shader
   // Normal = mat3(transpose(inverse(model))) * vertexNormal;
    Normal = vertexNormal;
    
    
}

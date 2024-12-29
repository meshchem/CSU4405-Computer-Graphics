#version 330 core

in vec2 UV; // UV coordinates passed from the vertex shader
uniform sampler2D textureSampler; // Texture sampler for the current face

out vec3 finalColor;

void main() {
   finalColor = texture(textureSampler, UV).rgb; // Sample the texture using UV coordinates
}

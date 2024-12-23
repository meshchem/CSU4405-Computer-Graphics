#version 330 core

in vec3 FragPos;        // Fragment position in world space
in vec3 Normal;         // Normal in world space
in vec3 VertexColor;    // Color from vertex shader
in vec2 TexCoord;       // Texture coordinate

out vec4 FragColor;     // Final output color

// Uniforms
uniform vec3 lightPosition;   // Position of the light source
uniform vec3 lightIntensity;  // Intensity of the light
uniform sampler2D textureSampler; // Texture sampler

// Attenuation parameters
uniform float constant;       // Constant attenuation factor
uniform float linear;         // Linear attenuation factor
uniform float quadratic;      // Quadratic attenuation factor

void main() {
    // Normalize the input normal and calculate light direction
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPosition - FragPos);

    // Calculate distance to the light sour
    float distance = length(lightPosition - FragPos);

    // Attenuation factor
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0); // Angle-based lighting
    vec3 diffuse = diff * lightIntensity * attenuation;

    // Combine lighting with texture
    vec3 textureColor = texture(textureSampler, TexCoord).rgb;
    vec3 finalColor = diffuse * textureColor;

    // Output final color
    FragColor = vec4(finalColor, 1.0);
}

#version 330 core

in vec2 TexCoord;       // Texture coordinates from vertex shader
in vec3 FragPos;        // Fragment position in world space
in vec3 Normal;         // Normal vector in world space

out vec4 FragColor;     // Final output color

// Uniforms
uniform sampler2D texture1; // Texture sampler
uniform vec3 lightDir;      // Directional light direction (normalized)
uniform vec3 lightColor;    // Light color
uniform vec3 viewPos;       // Camera position in world space

void main() {
    // Normalize the normal vector
    vec3 norm = normalize(Normal);

    // Ambient lighting
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting (Lambertian reflectance)
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (Blinn-Phong model)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(viewDir - lightDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting components
    vec3 lighting = ambient + diffuse + specular;

    // Sample the texture
    vec3 textureColor = texture(texture1, TexCoord).rgb;

    // Combine texture with lighting
    vec3 result = lighting * textureColor;

    // Output the final color
    FragColor = vec4(result, 1.0);
}

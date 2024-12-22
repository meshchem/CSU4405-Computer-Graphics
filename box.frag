#version 330 core

// Inputs from the vertex shader
in vec2 uv;             // Texture coordinates
in vec3 fragPos;        // Fragment position in world space
in vec3 normal;         // Normal vector in world space

// Output color
out vec4 finalColor;

// Uniforms
uniform sampler2D textureSampler; // Texture sampler
uniform vec3 lightDir;            // Directional light direction (normalized)
uniform vec3 lightColor;          // Light color
uniform vec3 viewPos;             // Camera position in world space

void main()
{
    // Normalize the normal vector
    vec3 norm = normalize(normal);

    // Ambient lighting
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting (Lambertian reflectance)
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular lighting (Blinn-Phong)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(viewDir - lightDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting components
    vec3 lighting = ambient + diffuse + specular;

    // Sample the texture
    vec3 textureColor = texture(textureSampler, uv).rgb;

    // Combine texture color with lighting
    vec3 result = lighting * textureColor;

    // Output the final color
    finalColor = vec4(result, 1.0);
}


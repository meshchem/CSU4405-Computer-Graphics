#version 330 core

in vec2 uv;
in vec3 Normal;
in vec3 FragPos; 
in vec4 FragPosLightSpace; // Light space position passed from vertex shader

uniform sampler2D texture1;     // Diffuse texture
uniform sampler2D shadowMap;    // Shadow map
uniform vec3 lightPos; 
uniform vec3 lightColor;        // Color of the light

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace) {
    // Perform perspective divide to get normalized device coordinates
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform from NDC to [0, 1]

    // Depth value from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // Current fragment's depth
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(Normal, normalize(lightPos - FragPos))), 0.0005);

    // Check if the fragment is in shadow
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    // Return shadow factor
    return shadow;
}

void main() {
    // Ambient lighting
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; 

    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);

    // Combine results
    vec3 lighting = ambient + (1.0 - shadow) * diffuse;

    // Apply texture
    vec3 textureColor = texture(texture1, uv).rgb;
    vec3 finalColor = lighting * textureColor;

    FragColor = vec4(finalColor, 1.0);
}

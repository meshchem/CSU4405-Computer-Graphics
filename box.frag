#version 330 core

in vec2 uv; 
in vec3 color;
in vec3 Normal;
in vec3 FragPos;  

uniform sampler2D textureSampler;  
uniform vec3 lightPos; 
uniform vec3 lightColor;      // Color of the light
uniform sampler2D shadowMap;  // Shadow map texture
uniform mat4 lightSpaceMatrix; // Light-space transformation matrix

out vec4 finalColor;

float calculateShadow(vec4 fragPosLightSpace) {
    // Transform from NDC to texture space
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Sample shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = 0.005;
    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}


void main()
{
   // finalColor = vec4(1.0, 1.0, 1.0, 1.0); // White

    
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos); 

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; 

    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // Calculate shadow factor
    float shadow = calculateShadow(fragPosLightSpace);

    // Combine lighting with shadow factor
    vec3 result = (ambient + (1.0 - shadow) * diffuse) * color;

    // Apply texture
    result *= texture(textureSampler, uv).rgb;
    
    finalColor = vec4(result, 1.0);
    
}

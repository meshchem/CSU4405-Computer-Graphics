#version 330 core

in vec3 color;
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;

uniform sampler2D textureSampler;
uniform vec3 lightDirection;
uniform vec3 lightIntensity;

out vec3 finalColor;

void main()

{	// Normalize the input normal  
	vec3 normal = normalize(worldNormal);

    // Calculate light direction
	vec3 lightDirNorm = normalize(lightDirection - worldPosition);

	// Lambertian Shading
	float diff = max(dot(normal, lightDirNorm), 0);
	vec3 diffuse = lightIntensity * diff * color;

	// Apply Attenuation based on distance
    float distance = length(lightDirection - worldPosition);
    float attenuation = 1.0 / (distance * distance);  // Quadratic attenuation
    diffuse *= attenuation;


    // Tone Mapping (Reinhard Operator)
    vec3 toneMappedColor = diffuse / (diffuse + vec3(1.0));

	finalColor = texture(textureSampler, uv).rgb * pow(toneMappedColor, vec3(1.0 / 2.2));  //* texture(textureSampler, uv).rgb;
}

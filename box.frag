#version 330 core

in vec2 uv; 
in vec3 color;
in vec3 Normal;
in vec3 FragPos;  

uniform sampler2D textureSampler;  
uniform vec3 lightPos; 
uniform vec3 lightColor;      // Color of the light

out vec4 finalColor;

void main()
{
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos); 

	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; 


    vec3 result = (ambient + diffuse) * color;

    result *= texture(textureSampler, uv).rgb;
    
    finalColor = vec4(result, 1.0);

    //finalColor = color;
	//finalColor  = texture(textureSampler, uv).rgb;
}

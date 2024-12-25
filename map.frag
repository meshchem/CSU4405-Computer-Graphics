#version 330 core

in vec2 uv;
in vec3 Normal;
in vec3 FragPos; 

uniform sampler2D texture1;
uniform vec3 lightPos; 
uniform vec3 lightColor;      // Color of the light

out vec4 FragColor;

void main() {

    //FragColor = texture(texture1, uv);
    //vec3 norm = normalize(Normal);
    //FragColor = vec4(norm * 0.5 + 0.5, 1.0);

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos); 

	float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor; 

    vec3 result = (ambient + diffuse);

    result *= texture(texture1, uv).rgb;
    
    FragColor = vec4(result, 1.0);
    
}

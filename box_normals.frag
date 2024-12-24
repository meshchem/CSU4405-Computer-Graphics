#version 330 core

in vec2 uv; 
in vec3 color;
in vec3 BuildingNormal;


uniform sampler2D textureSampler;  
  


out vec4 finalColor;

void main()
{
	//finalColor = color;

    vec3 normalizedNormal = normalize(BuildingNormal);
     finalColor = vec4(normalizedNormal * 0.5 + 0.5, 1.0);


	//finalColor  = texture(textureSampler, uv).rgb;
}

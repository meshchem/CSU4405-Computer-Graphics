#version 330 core

//in vec3 color;

in vec2 uv;  // TODO: To add UV input to this fragment shader 

uniform sampler2D textureSampler;  // TODO: To add the texture sampler

out vec3 finalColor;

void main()
{
	// finalColor = color;

	// TODO: texture lookup. 

	finalColor  = texture(textureSampler, uv).rgb;
}

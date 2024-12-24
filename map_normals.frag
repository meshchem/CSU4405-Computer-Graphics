#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos; 

out vec4 FragColor;

uniform sampler2D texture1;

void main() {

    //FragColor = texture(texture1, TexCoord);
    vec3 norm = normalize(Normal);
    FragColor = vec4(norm * 0.5 + 0.5, 1.0);

}

#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D ourTexture;
void main() {
    //FragColor = texture(ourTexture, TexCoords);
	FragColor = vec4(0.0, 0.8, 0.2, 1.0);
}

/*#version 330 core

out vec4 FragColor;

void main() {
	FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green color
}
*/
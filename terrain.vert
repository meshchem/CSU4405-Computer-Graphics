#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;
uniform mat4 MVP;
out vec2 uv;
void main() {
	gl_Position = MVP * vec4(aPos, 1.0);
        uv = vertexUV;
}


/*

#version 330 core

layout(location = 0) in vec3 aPos;
uniform mat4 viewProj;

void main() {
	gl_Position = viewProj * vec4(aPos, 1.0);
}

*/

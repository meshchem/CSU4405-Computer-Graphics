#version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoords;
        uniform mat4 viewProj;
        out vec2 TexCoords;
        void main() {
            gl_Position = viewProj * vec4(aPos, 1.0);
            TexCoords = aTexCoords;
        }


/*

#version 330 core

layout(location = 0) in vec3 aPos;
uniform mat4 viewProj;

void main() {
	gl_Position = viewProj * vec4(aPos, 1.0);
}

*/
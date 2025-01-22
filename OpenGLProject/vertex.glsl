#version 330 core

layout (location = 0) in vec3 aPos; // Position des Vertex
layout (location = 1) in vec2 aTexCoord; // Texturkoordinaten

uniform mat4 model; // Model-Matrix
uniform mat4 projection; // Orthogonale Projektion

out vec2 TexCoord; // Weitergabe der Texturkoordinaten

void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

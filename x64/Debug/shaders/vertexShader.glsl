#version 330 core

layout(location = 0) in vec2 aPos;       // 2D position (in object space)
layout(location = 1) in vec2 aTexCoord;  // texture coords

uniform mat4 uMVP;       // projection * view * model

out vec2 vTexCoord;
out vec2 vScreenPos;

void main()
{
    // We apply the MVP to convert from object space to clip space
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);

    // Pass the texture coords straight through
    vTexCoord = aTexCoord;

    // We'll store the original 2D position in vScreenPos
    // but typically you'd compute actual screen coords in the fragment
    // with gl_FragCoord. This is just a placeholder if you want it.
    vScreenPos = aPos;
}

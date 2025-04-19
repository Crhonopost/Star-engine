#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;

void main()
{
    TexCoords = aPos;
    vec4 pos = p * v * model * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  
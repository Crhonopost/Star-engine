#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aCoord;

out vec3 position;
out vec2 coord;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;

void main()
{
    position = aPos;
    coord = aCoord;
    vec4 pos = p * v * model * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}  
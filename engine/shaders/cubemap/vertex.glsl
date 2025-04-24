#version 330 core

layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 texCoord;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;

out vec3 TexCoords;

void main(){
        TexCoords = vertices_position_modelspace;

        gl_Position = vec4(vertices_position_modelspace,1);
        gl_Position = p * v * model *  gl_Position;

}


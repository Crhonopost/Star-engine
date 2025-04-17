#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;

uniform mat4 v;
uniform mat4 p;
uniform mat4 model;


void main(){
        gl_Position = vec4(vertices_position_modelspace,1);
        gl_Position = p * v * model *  gl_Position;
}


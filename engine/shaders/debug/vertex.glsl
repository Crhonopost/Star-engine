#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;


uniform vec3 scale = vec3(1);

uniform mat4 v;
uniform mat4 p;
uniform mat4 model;


void main(){
        vec3 scaledPos = vec3(vertices_position_modelspace.x * scale.x, vertices_position_modelspace.y * scale.y, vertices_position_modelspace.z * scale.z);
        gl_Position = vec4(scaledPos,1);
        gl_Position = p * v * model *  gl_Position;
}


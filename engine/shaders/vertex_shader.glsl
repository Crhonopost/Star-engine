#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 vNormal;

uniform mat4 vp;
uniform mat4 model;

out vec2 texCoords;
out vec3 WorldPos;
out vec3 normal;

void main(){
        texCoords = texCoord;

        gl_Position = vec4(vertices_position_modelspace,1);
        gl_Position = vp * model *  gl_Position;

        normal = normalize(mat3(transpose(inverse(model))) * vNormal);
        WorldPos = vec3(model * vec4(vertices_position_modelspace, 1.0));
}


#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 vNormal;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;

out vec2 texCoords;
out vec4 position;
out vec3 WorldPos;
out vec3 normalVal;
out vec3 camPos;

void main(){
        texCoords = texCoord;

        gl_Position = vec4(vertices_position_modelspace,1);
        gl_Position = p * v * model *  gl_Position;
        position = gl_Position;

        camPos = vec3(inverse(v)[3]);

        normalVal = normalize(mat3(transpose(inverse(model))) * vNormal);
        WorldPos = vec3(model * vec4(vertices_position_modelspace, 1.0));
}


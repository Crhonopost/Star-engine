#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 texCoord;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;
// uniform float time;

uniform sampler2D heightMap;

out vec2 texCoords;
out float height;

void main(){
        texCoords = texCoord;
        height = texture(heightMap, texCoords).r;

        gl_Position = vec4(vertices_position_modelspace,1);
        // gl_Position.y += height * 20.f;
        gl_Position = p * v * model *  gl_Position;
        // gl_Position.y += height * 4; //sin(time + texCoord.x * 2) / 2;

}


#version 330 core
out vec4 FragColor;

in vec3 position;
in vec2 coord;

uniform samplerCube skybox;
uniform bool octahedralProjection = false;


#include "../include/octahedral.glsl"


void main()
{
    if(octahedralProjection){
    	vec3 co = octahedral_unmapping(coord);
        FragColor = texture(skybox, co);
    } else {
        FragColor = texture(skybox, position);
    }
}
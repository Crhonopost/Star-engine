#version 330 core

uniform vec4 albedo = vec4(1, 0.7, 0.75, 1);

out vec4 color;

void main(){
    color = albedo;
}

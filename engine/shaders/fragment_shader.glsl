#version 330 core

// Ouput data
in vec2 texCoords;
out vec4 color;

uniform sampler2D tex;


void main(){
    color = vec4(texture(tex, texCoords).rgb, 1);
    // color = vec4(texCoords, 0, 1);
}

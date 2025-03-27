#version 330 core

// Ouput data
in vec2 texCoords;
out vec4 color;

uniform sampler2D tex;


void main(){
    color = texture(tex, texCoords);
    // color = vec4(texCoords, 0, 1);
}

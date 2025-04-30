#version 330 core

// Ouput data
in vec2 texCoords;
in vec3 WorldPos;
in vec4 position;
in vec3 normalVal;
in vec3 camPos;


out vec4 color;


uniform int renderMode = 0;

void main(){
    if(renderMode == 0){
        color = vec4(normalVal, 1);
    } else if(renderMode == 1){
        color = vec4(vec3(gl_FragCoord.z), 1);
    } else {
        color = vec4(texCoords, 1,1);
    }
}
#version 330 core

// Ouput data
in vec2 texCoords;
in float height;
out vec4 color;

vec2 pixelSize = vec2(0.005);
float grassScale = 8.0;
float snowHeight = 0.3;
float maxHeight = 0.8;

uniform sampler2D texGrass;
uniform sampler2D texRock;
uniform sampler2D heightMap;

float tiltDegree(float height, vec2 coords, vec2 direction){
    float otherHeight = texture(heightMap, coords + direction).r;
    return abs(height - otherHeight);
}

void main(){
    vec4 grassColor = texture(texGrass, texCoords * grassScale);
    vec4 rockColor = texture(texRock, texCoords);
    vec4 snowColor = vec4(1);

    
    float tiltX = texture(heightMap, texCoords + vec2(pixelSize.x, 0)).r - texture(heightMap, texCoords - vec2(pixelSize.x, 0)).r;
    float tiltY = texture(heightMap, texCoords + vec2(0, pixelSize.y)).r - texture(heightMap, texCoords - vec2(0, pixelSize.y)).r;
    float finalTilt = sqrt(tiltX * tiltX + tiltY * tiltY);
    finalTilt = min(finalTilt * 10.0, 1.0);

    /* Pour une interpollation entre herbe, roche et neige en fonction de la hauteur, j'avais fait:
        if(height < 0.5f){
            color = mix(grassColor, rockColor, height);
        } else {
            color = mix(rockColor, snowColor, (height - 0.5f) * 2.f);
        }
    */


    vec4 heightColor = mix(grassColor, snowColor, height / (snowHeight));

    color = mix(heightColor, rockColor, min(finalTilt, 1.f));
}

#version 330 core

// Ouput data
in vec2 texCoords;
in float height;
out vec4 color;

uniform vec2 pixelSize;
uniform sampler2D texGrass;
uniform sampler2D texRock;
uniform sampler2D texSnow;
uniform sampler2D heightMap;

float tiltDegree(float height, vec2 coords, vec2 direction){
    float otherHeight = texture(heightMap, coords + direction).r;
    return abs(height - otherHeight);
}

void main(){
    vec4 grassColor = texture(texGrass, texCoords);
    vec4 rockColor = texture(texRock, texCoords);
    vec4 snowColor = vec4(1);

    
    // float avgTilt = 0.f;
    // avgTilt += tiltDegree(height, texCoords, vec2(pixelSize.x, 0));
    // avgTilt += tiltDegree(height, texCoords, vec2(- pixelSize.x, 0));
    // avgTilt += tiltDegree(height, texCoords, vec2(0, pixelSize.y));
    // avgTilt += tiltDegree(height, texCoords, vec2(0, - pixelSize.y));

    // avgTilt /= 4.f;

    // color = vec4(avgTilt);

    if(height < 0.5f){
        color = mix(grassColor, rockColor, height * 2.f);

    } else {
        color = mix(rockColor, snowColor, (height - 0.5f) * 2.f);
    }
}

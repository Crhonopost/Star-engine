#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 vNormal;

layout(location = 3) in vec4 weights;
layout(location = 4) in ivec4 indices;

uniform mat4 p;
uniform mat4 v;
uniform mat4 model;

uniform mat4 bones[128];

out vec2 texCoords;
out vec3 WorldPos;
out vec3 normalVal;
out vec3 camPos;

void skinning(inout vec3 p, inout vec3 n, vec4 weights, ivec4 indices) {
    float sumWeights = weights.x + weights.y + weights.z + weights.w;
    
    if (sumWeights > 0.0) {
        // 1. Normaliser les poids (s'ils ne le sont pas déjà)
        weights /= sumWeights;

        // 2. Combinaison linéaire des matrices de bone
        mat4 m = 
            bones[indices.x] * weights.x +
            bones[indices.y] * weights.y +
            bones[indices.z] * weights.z +
            bones[indices.w] * weights.w;

        // 3. Appliquer au vertex et à la normale
        p = (m * vec4(p, 1.0)).xyz;
        n = normalize(transpose(inverse(mat3(m))) * n);
    }
}

void main(){
    texCoords = texCoord;

    vec3 pos = vertices_position_modelspace;
    vec3 normal = vNormal;

    skinning(pos, normal, weights, indices);

    gl_Position = vec4(pos,1);
    gl_Position = p * v * model *  gl_Position;

    camPos = vec3(inverse(v)[3]);

    normalVal = normalize(mat3(transpose(inverse(model))) * vNormal);
    WorldPos = vec3(model * vec4(vertices_position_modelspace, 1.0));
}


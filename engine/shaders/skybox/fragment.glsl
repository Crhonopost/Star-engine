#version 330 core
out vec4 FragColor;

in vec3 position;
in vec2 coord;

uniform samplerCube skybox;
uniform bool octahedralProjection = false;

vec3 octahedral_unmapping(vec2 co)
{
    co = co * 2.0 - 1.0;

    vec2 abs_co = abs(co);
    vec3 v = vec3(co, 1.0 - (abs_co.x + abs_co.y));

    if ( abs_co.x + abs_co.y > 1.0 ) {
        v.xy = (abs(co.yx) - 1.0) * -sign(co.xy);
    }

    return v;
}



void main()
{
    if(octahedralProjection){
    	vec3 co = octahedral_unmapping(coord);
        FragColor = texture(skybox, co);
    } else {
        FragColor = texture(skybox, position);
    }
}
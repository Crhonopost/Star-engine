#version 330 core

// Ouput data
in vec2 texCoords;
in vec3 WorldPos;
in vec3 normalVal;
out vec4 color;

uniform vec3 albedoVal = vec3(1.0,0.f,0.f);
uniform float metallicVal = 0.5f;
uniform float roughnessVal = 0.5f;
uniform float aoVal = 1.f;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform samplerCube irradianceMap;

// lights
const int MAX_LIGHT = 20;
uniform int lightCount = 0;
uniform vec3 lightPositions[MAX_LIGHT];// = vec3[MAX_LIGHT](vec3(2.f,2.f,0.f));
uniform vec3 lightColors[MAX_LIGHT];// = vec3[MAX_LIGHT](vec3(1.f));
uniform float indensiteScaleLight = 1.f;


in vec3 camPos;

uniform bool hasTexture = false;

const float PI = 3.14159265359;

/*--------------------------------------PBR--------------------------------------*/
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 getNormalFromNormalMap(){
    return -normalize(texture2D(normalMap,texCoords).rgb*2.0-1.0);
}
/*--------------------------------------PBR--------------------------------------*/

void main(){
    vec3 albedo,normal;
    float metallic,roughness,ao;

    if(hasTexture){
        albedo      = pow(texture(albedoMap, texCoords).rgb, vec3(2.2f));
        normal      = getNormalFromNormalMap();
        metallic   = texture(metallicMap, texCoords).r;
        roughness  = texture(roughnessMap, texCoords).r ;
        ao         = texture(aoMap, texCoords).r;
    }else{
        albedo = pow(albedoVal,vec3(2.2f));
        normal = normalVal;
        metallic = metallicVal;
        roughness = roughnessVal;
        ao = aoVal;
    }

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < lightCount; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(lightPositions[i] - WorldPos);
        float attenuation = 1.f / (distance * distance);
        vec3 radiance     = lightColors[i] * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += indensiteScaleLight * (kD * albedo / PI + specular) * radiance * NdotL; 
    }
    // vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;
    vec3 colorPBR = ambient + Lo;
	
    colorPBR = colorPBR / (colorPBR + vec3(1.0));
    colorPBR = pow(colorPBR, vec3(1.0/2.2));  


    color = vec4(colorPBR, 1.0);
}

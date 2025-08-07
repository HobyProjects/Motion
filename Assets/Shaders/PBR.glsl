#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;

out vec3 v_WorldPosition;
out vec2 v_UV;
out vec3 v_Normal;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat3 u_NormalMatrix;

// Displacement Mapping
uniform sampler2D u_DisplacementTextures;

struct MaterialAttributes {
    vec3 BaseColor;
    float Metallic;
    float Roughness;
    float Opacity;
};

layout(std430, binding = 0) buffer MaterialData {
    MaterialAttributes attributes;
};

void main()
{
    float displacement = texture(u_DisplacementTextures, a_TexCoords).r;
    vec3 displacedPosition = a_Position + a_Normals * displacement;

    v_WorldPosition = vec3(u_ModelMatrix * vec4(displacedPosition, 1.0));
    v_Normal = u_NormalMatrix * a_Normals;
    v_UV = a_TexCoords;

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(v_WorldPosition, 1.0);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPosition;
in vec2 v_UV;
in vec3 v_Normal;

// Texture Samplers
uniform sampler2D u_BaseColorTextures;
uniform sampler2D u_MetallicTextures;
uniform sampler2D u_RoughnessTextures;
uniform sampler2D u_AmbientOcclusionTextures;
uniform sampler2D u_NormalTextures;

// IBL
uniform samplerCube u_IrradianceTextures;
uniform samplerCube u_PrefilteredTextures;
uniform sampler2D u_BRDFLUT;

#define MAX_LIGHTS 4 
#define PI 3.14159265359

// Light & Camera
uniform vec3 u_CameraPosition;
uniform vec3 u_LightPosition[MAX_LIGHTS];
uniform vec3 u_LightColor[MAX_LIGHTS];
uniform float u_LightIntensity[MAX_LIGHTS];

// Material Buffer
struct MaterialAttributes {
    vec3 BaseColor;
    float Metallic;
    float Roughness;
    float Opacity;
};

layout(std430, binding = 0) buffer MaterialData {
    MaterialAttributes attributes;
};

vec3 GetNormalFromTexture()
{
    vec3 tangentNormal = texture(u_NormalTextures, v_UV).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(v_WorldPosition);
    vec3 Q2  = dFdy(v_WorldPosition);
    vec2 st1 = dFdx(v_UV);
    vec2 st2 = dFdy(v_UV);

    vec3 N   =  normalize(v_Normal);
    vec3 T   =  normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN =  mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float A = roughness * roughness;
    float A2 = A * A;

    float NdotH1 = max(dot(N, H), 0.0);
    float NdotH2 = NdotH1 * NdotH1;

    float NOM = A2;
    float DENOM = (NdotH2 * (A2 - 1.0) + 1.0);
    DENOM = PI * DENOM * DENOM;

    return NOM / DENOM;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float R = roughness + 1.0;
    float K = (R * R) / 8.0;

    float NOM = NdotV;
    float DENOM = NdotV * (1.0 - K) + K;

    return NOM / DENOM;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NDOTV = max(dot(N, V), 0.0);
    float NDOTL = max(dot(N, L), 0.0);

    float GGX1 = GeometrySchlickGGX(NDOTV, roughness);
    float GGX2 = GeometrySchlickGGX(NDOTL, roughness);
    return GGX1 * GGX2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main()
{
    // material properties
    vec3 ALBEDO    = pow(texture(u_BaseColorTextures, v_UV).rgb, vec3(2.2)) * attributes.BaseColor;
    float METALLIC = texture(u_MetallicTextures, v_UV).r * attributes.Metallic;
    float ROUGHNESS = texture(u_RoughnessTextures, v_UV).r * attributes.Roughness;
    float AO = texture(u_AmbientOcclusionTextures, v_UV).r;

    // INPUT LIGHTING DATA
    vec3 N = GetNormalFromTexture();
    vec3 V = normalize(u_CameraPosition - v_WorldPosition);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, ALBEDO, METALLIC);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < MAX_LIGHTS; i++)
    {
        // calculate per-light radiance
        vec3 L = normalize(u_LightPosition[i] - v_WorldPosition);
        vec3 H = normalize(V + L);
        float distance = length(u_LightPosition[i] - v_WorldPosition);
        float attenuation = u_LightIntensity[i] / (distance * distance);
        vec3 radiance = u_LightColor[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, ROUGHNESS);   
        float G   = GeometrySmith(N, V, L, ROUGHNESS);    
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
    
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - METALLIC;	                
            
        float NdotL = max(dot(N, L), 0.0);        
        Lo += (kD * ALBEDO / PI + specular) * radiance * NdotL;
    }

    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, ROUGHNESS);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - METALLIC);

    // IBL Ambient
    vec3 irradiance = texture(u_IrradianceTextures, N).rgb;
    vec3 diffuse = irradiance * ALBEDO;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefiltered = textureLod(u_PrefilteredTextures, R, ROUGHNESS * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), ROUGHNESS)).rg;
    vec3 specular = prefiltered * (F * brdf.x + brdf.y);
    vec3 ambient = (kD * diffuse + specular) * AO;
    vec3 color = Lo + ambient;

    // Final color
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2)); 
    FragColor = vec4(color, attributes.Opacity);
}

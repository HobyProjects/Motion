#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;
layout(location = 3) in vec3 a_Tangents;
layout(location = 4) in vec3 a_Bitangents;

layout(location = 0) out vec3 v_WorldPosition;
layout(location = 1) out vec2 v_UV;
layout(location = 2) out vec3 v_Normal;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat4 u_ModelMatrix;

void main()
{
    v_UV = a_TexCoords;
    v_WorldPosition = vec3(u_ModelMatrix * vec4(a_Position, 1.0));
    v_Normal = transpose(inverse(mat3(u_ModelMatrix))) * a_Normals;
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(v_WorldPosition, 1.0);
}


#type fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 v_WorldPosition;
layout(location = 1) in vec2 v_UV;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in mat3 v_TBN;

// PBR texture uniforms
uniform sampler2D u_BaseColorTextures;
uniform sampler2D u_MetallicTextures;
uniform sampler2D u_RoughnessTextures;
uniform sampler2D u_AmbientOcclusionTextures;
uniform sampler2D u_NormalTextures;

uniform samplerCube u_EnvironmentTexture;

// Light and camera
uniform vec3 u_CameraPosition;
uniform vec3 u_LightPosition;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

// Material attributes from shader storage buffer
struct MaterialAttributes
{
    vec3 BaseColor;
    float Metallic;
    float Roughness;
    float AmbientOcclusion;
    float Opacity;
    float DisplacementScale;
    float PADDING1;
    float PADDING2;
};

layout(std430, binding = 0) buffer MaterialData
{
    MaterialAttributes attributes;
};

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(u_NormalTextures, v_UV).xyz * 2.0 - 1.0;
    return normalize(v_TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(dot(N, V), roughness) * GeometrySchlickGGX(dot(N, L), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 albedo = pow(texture(u_BaseColorTextures, v_UV).rgb, vec3(2.2)) * attributes.BaseColor;
    float metallic = texture(u_MetallicTextures, v_UV).r * attributes.Metallic;
    float roughness = texture(u_RoughnessTextures, v_UV).r * attributes.Roughness;
    float ao = texture(u_AmbientOcclusionTextures, v_UV).r * attributes.AmbientOcclusion;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(u_CameraPosition - v_WorldPosition);
    vec3 R = reflect(-V, N);

    vec3 L = normalize(u_LightPosition - v_WorldPosition);
    vec3 H = normalize(V + L);
    float distance = length(u_LightPosition - v_WorldPosition);
    float attenuation = u_LightIntensity / (distance * distance);
    vec3 radiance = u_LightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F0   = mix(vec3(0.04), albedo, metallic);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denom       = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular     = numerator / denom;

    float NdotL = max(dot(N, L), 0.0);
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // IBL (basic) ambient light
    vec3 ambient = texture(u_EnvironmentTexture, N).rgb * albedo * ao;

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2)); // gamma correction

    FragColor = vec4(color, attributes.Opacity);
}

#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;
layout(location = 3) in vec4 a_Tangent;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjMatrix;

out vec3 v_WorldPosition;
out vec2 v_UV;
out mat3 v_TBN;

void main() {
    vec3 T = normalize(mat3(u_ModelMatrix) * a_Tangent.xyz);
    vec3 N = normalize(mat3(u_ModelMatrix) * a_Normals);
    vec3 B = normalize(cross(N, T)) * a_Tangent.w;

    v_TBN = mat3(T, B, N);
    v_UV = a_TexCoords;
    v_WorldPosition = vec3(u_ModelMatrix * vec4(a_Position, 1.0));

    gl_Position = u_ViewProjMatrix * vec4(v_WorldPosition, 1.0);
}

#type fragment
#version 460 core
#define MAX_MATERIAL_LAYERS 4

in vec3 v_WorldPos;
in vec2 v_TexCoords;
in mat3 v_TBN;

out vec4 FragColor;

uniform vec3 u_CameraPosition;
uniform vec3 u_LightPosition;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

layout(std430, binding = 0) buffer MaterialLayerBuffer {
    struct MaterialLayerData {
        vec3 BaseColorFactor;
        float MetallicFactor;
        float RoughnessFactor;
        float Opacity;
        float AOFactor;
        float ClearCoatFactor;
        float ClearCoatRoughness;
        float SheenFactor;
        float SheenRoughness;
        float Transmission;
        float IOR;
        float BlendFactor;
    };
    MaterialLayerData MaterialLayers[MAX_MATERIAL_LAYERS];
};

// Texture arrays
uniform sampler2D u_BaseColorTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_MetallicTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_RoughnessTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_AmbientOcclusionTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_NormalTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_OpacityTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_ClearCoatTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_SheenTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_TransmissionTextures[MAX_MATERIAL_LAYERS];
uniform sampler2D u_BlendMasks[MAX_MATERIAL_LAYERS];

uniform vec3 u_EmissiveColor;
uniform sampler2D u_EmissiveTexture;
uniform samplerCube u_EnvironmentTexture;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getNormal() {
    vec3 blendedNormal = vec3(0.0);
    for (int i = 0; i < MAX_MATERIAL_LAYERS; ++i) {
        float blend = texture(u_BlendMasks[i], v_TexCoords).r * MaterialLayers[i].BlendFactor;
        vec3 nrm = texture(u_NormalTextures[i], v_TexCoords).rgb * 2.0 - 1.0;
        blendedNormal += nrm * blend;
    }
    return normalize(v_TBN * blendedNormal);
}

void main() {
    vec3 finalAlbedo = vec3(0.0);
    float finalMetallic = 0.0;
    float finalRoughness = 0.0;
    float finalAO = 0.0;
    float finalClearCoat = 0.0;
    float finalSheen = 0.0;
    float finalTransmission = 0.0;
    float finalOpacity = 0.0;
    float finalIOR = 1.0;

    for (int i = 0; i < MAX_MATERIAL_LAYERS; ++i) {
        MaterialLayerData layer = MaterialLayers[i];
        float blend = texture(u_BlendMasks[i], v_TexCoords).r * layer.BlendFactor;

        vec3 albedo = texture(u_BaseColorTextures[i], v_TexCoords).rgb * layer.BaseColorFactor;
        float metallic = texture(u_MetallicTextures[i], v_TexCoords).r * layer.MetallicFactor;
        float roughness = texture(u_RoughnessTextures[i], v_TexCoords).r * layer.RoughnessFactor;
        float ao = texture(u_AmbientOcclusionTextures[i], v_TexCoords).r * layer.AOFactor;
        float opacity = texture(u_OpacityTextures[i], v_TexCoords).r * layer.Opacity;
        float clearCoat = texture(u_ClearCoatTextures[i], v_TexCoords).r * layer.ClearCoatFactor;
        float sheen = texture(u_SheenTextures[i], v_TexCoords).r * layer.SheenFactor;
        float transmission = texture(u_TransmissionTextures[i], v_TexCoords).r * layer.Transmission;

        finalAlbedo = mix(finalAlbedo, albedo, blend);
        finalMetallic = mix(finalMetallic, metallic, blend);
        finalRoughness = mix(finalRoughness, roughness, blend);
        finalAO = mix(finalAO, ao, blend);
        finalOpacity = mix(finalOpacity, opacity, blend);
        finalClearCoat = mix(finalClearCoat, clearCoat, blend);
        finalSheen = mix(finalSheen, sheen, blend);
        finalTransmission = mix(finalTransmission, transmission, blend);
        finalIOR = mix(finalIOR, layer.IOR, blend);
    }

    vec3 N = getNormal();
    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 L = normalize(u_LightPosition - v_WorldPos);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 F0 = mix(vec3(0.04), finalAlbedo, finalMetallic);
    vec3 F = fresnelSchlick(HdotV, F0);
    float D = pow(NdotH, (1.0 - finalRoughness) * 128.0);
    float G = max(NdotL * NdotV, 0.001);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - finalMetallic);
    vec3 diffuse = finalAlbedo / PI;
    vec3 specular = D * G * F / max(4.0 * NdotL * NdotV, 0.001);
    vec3 lightRadiance = u_LightColor * u_LightIntensity;
    vec3 color = (kD * diffuse + specular) * lightRadiance * NdotL;

    vec3 emissive = texture(u_EmissiveTexture, v_TexCoords).rgb * u_EmissiveColor;
    color += emissive;

    color = mix(color, vec3(1.0), finalClearCoat * 0.2);
    color += finalSheen * vec3(1.0) * 0.05;
    color = mix(color, vec3(1.0), finalTransmission * 0.5);

    vec3 R = reflect(-V, N);
    vec3 envReflection = texture(u_EnvironmentTexture, R).rgb;
    float reflectionStrength = mix(1.0, 0.0, finalRoughness);
    color += envReflection * F * reflectionStrength;

    color *= finalAO;

    FragColor = vec4(color, finalOpacity);
}


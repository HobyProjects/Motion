#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;
layout(location = 3) in vec3 a_Tangents;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjMatrix;

out vec3 v_WorldPos;
out vec2 v_TexCoords;
out mat3 v_TBN;

void main() {
    vec3 T = normalize(mat3(u_ModelMatrix) * a_Tangents);
    vec3 N = normalize(mat3(u_ModelMatrix) * a_Normals);
    vec3 B = cross(N, T);

    v_TBN = mat3(T, B, N);
    v_TexCoords = a_TexCoords;
    v_WorldPos = vec3(u_ModelMatrix * vec4(a_Position, 1.0));

    gl_Position = u_ViewProjMatrix * vec4(v_WorldPos, 1.0);
}

#type fragment
#version 460 core

in vec3 v_WorldPos;
in vec2 v_TexCoords;
in mat3 v_TBN;

out vec4 FragColor;

// Camera and lighting
uniform vec3 u_CameraPosition;
uniform vec3 u_LightPosition;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

// Material factors
uniform vec3 u_BaseColorFactor;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform float u_AmbientOcclusion;
uniform float u_ClearCoatFactor;
uniform float u_ClearCoatRoughnessFactor;
uniform float u_SheenFactor;
uniform float u_SheenRoughnessFactor;
uniform float u_TransmissionFactor;
uniform float u_IndexOfRefraction;

// Emissive
uniform vec3 u_EmissiveColor;

// Textures
uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_MetallicTexture;
uniform sampler2D u_RoughnessTexture;
uniform sampler2D u_AmbientOcclusionTexture;
uniform sampler2D u_NormalMapsTexture;
uniform sampler2D u_ClearCoatTexture;
uniform sampler2D u_SheenTexture;
uniform sampler2D u_TransmissionTexture;
uniform sampler2D u_EmissiveTexture;

// Environment
uniform samplerCube u_EnvironmentCubeMap;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getNormal() {
    vec3 normalMap = texture(u_NormalMapsTexture, v_TexCoords).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    return normalize(v_TBN * normalMap);
}

void main() {
    vec3 albedo = texture(u_BaseColorTexture, v_TexCoords).rgb * u_BaseColorFactor;
    float metallic = texture(u_MetallicTexture, v_TexCoords).r * u_MetallicFactor;
    float roughness = texture(u_RoughnessTexture, v_TexCoords).r * u_RoughnessFactor;
    float ao = texture(u_AmbientOcclusionTexture, v_TexCoords).r * u_AmbientOcclusion;

    vec3 N = getNormal();
    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 L = normalize(u_LightPosition - v_WorldPos);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // BRDF terms
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(HdotV, F0);

    float D = pow(NdotH, (1.0 - roughness) * 128.0); // simplified
    float G = max(NdotL * NdotV, 0.001);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = albedo / PI;

    vec3 specular = D * G * F / max(4.0 * NdotL * NdotV, 0.001);

    // Light contribution
    vec3 lightRadiance = u_LightColor * u_LightIntensity;
    vec3 color = (kD * diffuse + specular) * lightRadiance * NdotL;

    // Emissive
    vec3 emissive = texture(u_EmissiveTexture, v_TexCoords).rgb * u_EmissiveColor;
    color += emissive;

    // Clear coat
    float clearCoat = texture(u_ClearCoatTexture, v_TexCoords).r * u_ClearCoatFactor;
    color = mix(color, vec3(1.0), clearCoat * 0.2);

    // Sheen
    float sheen = texture(u_SheenTexture, v_TexCoords).r * u_SheenFactor;
    color += sheen * vec3(1.0) * 0.05;

    // Transmission (fake)
    float transmission = texture(u_TransmissionTexture, v_TexCoords).r * u_TransmissionFactor;
    color = mix(color, vec3(1.0), transmission * 0.5);

    // IBL reflection
    vec3 R = reflect(-V, N);
    vec3 envReflection = texture(u_EnvironmentCubeMap, R).rgb;
    float reflectionStrength = mix(1.0, 0.0, roughness);
    vec3 specEnv = envReflection * F * reflectionStrength;

    color += specEnv;

    // Apply AO
    color = mix(color, color * ao, 1.0);

    FragColor = vec4(color, 1.0);
}
#type vertex
#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in vec3 a_Normal;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

uniform mat4 u_ModelMatrix;
uniform mat4 u_CameraMatrix;

out vec3 v_WorldPos;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;
out vec3 v_Normal;

void main()
{
    vec4 worldPosition = u_ModelMatrix * vec4(a_Position, 1.0);
    v_WorldPos = worldPosition.xyz;
    v_TexCoord = a_TexCoord;

    mat3 normalMatrix = transpose(inverse(mat3(u_ModelMatrix)));
    v_Normal = normalize(normalMatrix * a_Normal);
    v_Tangent = normalize(normalMatrix * a_Tangent);
    v_Bitangent = normalize(normalMatrix * a_Bitangent);

    gl_Position = u_CameraMatrix * worldPosition;
}

#type fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPos;
in vec2 v_TexCoord;
in vec3 v_Tangent;
in vec3 v_Bitangent;
in vec3 v_Normal;

layout(std140, binding = 0) uniform CameraData {
    vec3 u_CameraPos;
};

layout(std140, binding = 1) uniform LightData {
    vec3 u_LightPosition;
    vec3 u_LightColor;
    float u_LightIntensity;
};

layout(std140, binding = 2) uniform MaterialFactors {
    vec3 u_BaseColor;
    float u_Metallic;
    float u_Roughness;
    float u_ClearCoat;
    float u_ClearCoatRoughness;
    float u_Sheen;
    float u_SheenRoughness;
    float u_AmbientOcclusion;
};

uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_MetallicTexture;
uniform sampler2D u_RoughnessTexture;
uniform sampler2D u_AmbientOcclusionTexture;
uniform sampler2D u_NormalMapsTexture;
uniform sampler2D u_ClearCoatTexture;
uniform sampler2D u_SheenTexture;

// IBL
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilteredEnvMap;
uniform sampler2D u_BRDFLUT;

const float PI = 3.14159265359;

vec3 getNormal()
{
    vec3 tangentNormal = texture(u_NormalMapsTexture, v_TexCoord).rgb * 2.0 - 1.0;
    mat3 TBN = mat3(v_Tangent, v_Bitangent, v_Normal);
    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return num / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 N = getNormal();
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    vec3 L = normalize(u_LightPosition - v_WorldPos);
    vec3 H = normalize(V + L);

    vec3 baseColor = pow(texture(u_BaseColorTexture, v_TexCoord).rgb * u_BaseColor, vec3(2.2));
    float metallic = texture(u_MetallicTexture, v_TexCoord).r * u_Metallic;
    float roughness = texture(u_RoughnessTexture, v_TexCoord).r * u_Roughness;
    float ao = texture(u_AmbientOcclusionTexture, v_TexCoord).r * u_AmbientOcclusion;
    float clearCoat = texture(u_ClearCoatTexture, v_TexCoord).r * u_ClearCoat;
    float clearCoatRoughness = u_ClearCoatRoughness;
    float sheen = texture(u_SheenTexture, v_TexCoord).r * u_Sheen;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    vec3 radiance = u_LightColor * u_LightIntensity / (length(u_LightPosition - v_WorldPos) * length(u_LightPosition - v_WorldPos));
    vec3 specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * NdotL + 0.001);

    vec3 Lo = (kD * baseColor / PI + specular) * radiance * NdotL;

    // IBL (Reflection approximation)
    vec3 F_ibl = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS_ibl = F_ibl;
    vec3 kD_ibl = (1.0 - kS_ibl) * (1.0 - metallic);

    vec3 irradiance = texture(u_IrradianceMap, N).rgb;
    vec3 diffuse = irradiance * baseColor;

    vec3 R = reflect(-V, N);
    vec3 prefilteredColor = textureLod(u_PrefilteredEnvMap, R, roughness * 4.0).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specularIBL = prefilteredColor * (F_ibl * brdf.x + brdf.y);

    vec3 ambient = (kD_ibl * diffuse + specularIBL) * ao;
    vec3 sheenTerm = sheen * (1.0 - metallic) * irradiance;
    vec3 clearCoatF = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));
    float ccNDF = DistributionGGX(N, H, clearCoatRoughness);
    float ccG = GeometrySmith(N, V, L, clearCoatRoughness);
    float ccSpec = clearCoat * ccNDF * ccG * dot(N, L);

    vec3 finalColor = ambient + Lo + sheenTerm + clearCoatF * ccSpec;
    finalColor = pow(finalColor, vec3(1.0 / 2.2)); // Gamma correction

    FragColor = vec4(finalColor, 1.0);
}

#type vertex
#version 460 core

layout (location = 0) in vec3 a_Positions;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;
layout (location = 3) in vec3 a_Tangents;
layout (location = 4) in vec3 a_Bitangents; // kept for interface parity, but we rebuild B

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat3 u_NormalMatrix;

out v_OUT
{
    vec3 WorldPosition;
    vec2 UVs;
    vec3 Normals;
    vec3 Tangents;
    vec3 Bitangents;
} VOUT;

void main()
{
    vec4 world         = u_ModelMatrix * vec4(a_Positions, 1.0);
    VOUT.WorldPosition = world.xyz;
    VOUT.UVs           = a_TextureCoords;

    // Robust, right-handed TBN: rebuild B
    vec3 N = normalize(u_NormalMatrix * a_Normals);
    vec3 T = normalize(u_NormalMatrix * a_Tangents);
    vec3 B = normalize(cross(N, T));

    VOUT.Tangents   = T;
    VOUT.Bitangents = B;
    VOUT.Normals    = N;

    gl_Position = u_ProjectionMatrix * u_ViewMatrix * world;
}

// ==========================================================================================================================

#type fragment
#version 460 core

layout (location = 0) out vec4 FragColor;

in v_OUT
{
    vec3 WorldPosition;
    vec2 UVs;
    vec3 Normals;
    vec3 Tangents;
    vec3 Bitangents;
} FIN;

struct SunLight
{
    vec3  Direction;
    vec3  Color;
    float Intensity;
};

struct Textures
{
    // Core
    sampler2D BaseColorTexture;
    sampler2D AOTexture;
    sampler2D MetallicTexture;
    sampler2D RoughnessTexture;
    sampler2D NormalMapTexture;
    sampler2D EmissiveTexture;
    sampler2D OpacityTexture;

    // Packed
    sampler2D ORMTexture; // R=AO, G=Roughness, B=Metallic

    // Clearcoat
    sampler2D ClearcoatTexture;
    sampler2D ClearcoatRoughnessTexture;

    // Specular (KHR_specular-style)
    sampler2D SpecularColorTexture;
    sampler2D SpecularTexture;

    // Sheen
    sampler2D SheenColorTexture;
    sampler2D SheenRoughnessTexture;

    // Transmission + Volume-ish
    sampler2D TransmissionTexture;
    sampler2D ThicknessTexture;
};

struct Attributes
{
    // Core
    vec3  BaseColor;
    float MetallicFactor;
    float RoughnessFactor;
    float Opacity;

    // Specular controls (dielectric only)
    vec3  SpecularColor;     // color tint for dielectric F0
    float SpecularLevel;     // scalar for dielectric F0

    // Clearcoat
    float ClearcoatFactor;
    float ClearcoatRoughnessFactor;

    // Sheen
    vec3  SheenColor;
    float SheenRoughnessFactor;

    // Transmission / Volume-ish
    float TransmissionFactor;
    float ThicknessFactor;
    vec3  AttenuationColor;
    float AttenuationDistance;

    // IOR (used for Fresnel and refraction)
    float IOR;
};

const int TB_BaseColor  = 1 << 0;
const int TB_Metallic   = 1 << 1;
const int TB_Roughness  = 1 << 2;
const int TB_Normal     = 1 << 3;
const int TB_AO         = 1 << 4;
const int TB_Emissive   = 1 << 5;
const int TB_Opacity    = 1 << 6;
const int TB_ORM        = 1 << 7;

const int TB_Clearcoat  = 1 << 8;
const int TB_ClearcoatR = 1 << 9;

const int TB_SpecColor  = 1 << 10;
const int TB_Spec       = 1 << 11;

const int TB_SheenColor = 1 << 12;
const int TB_SheenR     = 1 << 13;

const int TB_Trans      = 1 << 14;
const int TB_Thick      = 1 << 15;

const float PI  = 3.14159265359;
const float EPS = 1e-6;

uniform SunLight    u_SunLight;
uniform vec3        u_CameraPosition;
uniform float       u_NormalYFlip; // >0.5 => flip Y

uniform samplerCube u_IrradianceTexture;
uniform samplerCube u_PrefilteredTexture;
uniform sampler2D   u_BRDFLUTTexture;
uniform float       u_IBLIntensity_Diffuse;
uniform float       u_IBLIntensity_Specular;
uniform float       u_IBLMipLevels;  // if <=0, default used

uniform Textures    u_Textures;
uniform Attributes  u_Attributes;
uniform int         u_TextureBitmask;

bool HasTexture(int bitMask) { return (u_TextureBitmask & bitMask) != 0; }

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = max(roughness * roughness, 1e-4);
    float a2     = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom       = PI * denom * denom;

    return num / max(denom, EPS);
}

float GeometrySmithSchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySmithSchlickGGX(NdotV, rough);
    float ggx1  = GeometrySmithSchlickGGX(NdotL, rough);
    return ggx1 * ggx2;
}

vec3 ApplyNormalMaps(vec3 N, vec3 T, vec3 B, vec2 UVs)
{
    if(!HasTexture(TB_Normal)) return normalize(N);

    vec3 n = texture(u_Textures.NormalMapTexture, UVs).xyz * 2.0 - 1.0;
    if (u_NormalYFlip > 0.5) n.y = -n.y;

    // Column-major TBN: T, B, N
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
    return normalize(TBN * n);
}

// Sheen helpers
float DistributionCharlie(float NdotH, float alpha)
{
    float invAlpha = 1.0 / max(alpha, 1e-4);
    float cosh = clamp(NdotH, 0.0, 1.0);
    float sinh = sqrt(max(1.0 - cosh*cosh, 0.0));
    return (2.0 + invAlpha) * pow(sinh, invAlpha) / (2.0 * PI);
}

float VisibilitySheen(float NdotL, float NdotV)
{
    return 1.0 / max(4.0 * (NdotL + NdotV - NdotL*NdotV), EPS);
}

// Transmission helpers
vec3 RefractEnvironment(vec3 V, vec3 N, float ETA)
{
    return refract(-V, N, 1.0 / max(ETA, 1e-4));
}

vec3 Attenuation(vec3 attColor, float attDist, float distance)
{
    if (attDist <= 0.0) return vec3(1.0);
    vec3 sigma_a = -log(max(attColor, vec3(1e-4))) / max(attDist, 1e-4);
    return exp(-sigma_a * distance);
}

void main()
{
    vec2 UV = FIN.UVs;
    vec3 V  = normalize(u_CameraPosition - FIN.WorldPosition);
    vec3 N  = ApplyNormalMaps(FIN.Normals, FIN.Tangents, FIN.Bitangents, UV);

    // -------- Material inputs (defaults) --------
    vec3  baseColor        = u_Attributes.BaseColor;
    float metallicFactor   = clamp(u_Attributes.MetallicFactor, 0.0, 1.0);
    float roughnessFactor  = clamp(u_Attributes.RoughnessFactor, 1e-4, 1.0);
    float opacity          = clamp(u_Attributes.Opacity, 0.0, 1.0);
    float AO               = 1.0;

    // Clearcoat
    float clearcoatFactor          = clamp(u_Attributes.ClearcoatFactor, 0.0, 1.0);
    float clearcoatRoughnessFactor = clamp(u_Attributes.ClearcoatRoughnessFactor, 0.0, 1.0);

    // Specular controls
    vec3  specularColor = u_Attributes.SpecularColor;
    float specLevel     = u_Attributes.SpecularLevel;

    // Sheen
    vec3  sheenColor           = u_Attributes.SheenColor;
    float sheenRoughnessFactor = clamp(u_Attributes.SheenRoughnessFactor, 0.0, 1.0);

    // Transmission / Volume-ish
    float transmissionFactor = clamp(u_Attributes.TransmissionFactor, 0.0, 1.0);
    float thicknessFactor    = clamp(u_Attributes.ThicknessFactor, 0.0, 1.0);
    vec3  attenuationColor   = u_Attributes.AttenuationColor;
    float attenuationDistance= max(u_Attributes.AttenuationDistance, 1e-4);

    // IOR
    float IOR = (u_Attributes.IOR > 0.0) ? u_Attributes.IOR : 1.5;

    // -------- Textures --------
    if(HasTexture(TB_BaseColor))
        baseColor = texture(u_Textures.BaseColorTexture, UV).rgb * baseColor;

    if(HasTexture(TB_ORM) && !HasTexture(TB_Roughness) && !HasTexture(TB_Metallic))
    {
        vec3 ORM = texture(u_Textures.ORMTexture, UV).rgb; // R=AO, G=Roughness, B=Metallic
        metallicFactor  = clamp(ORM.b * metallicFactor, 0.0, 1.0);
        roughnessFactor = clamp(ORM.g * roughnessFactor, 1e-4, 1.0);
        AO              = ORM.r;
    }

    if(HasTexture(TB_Roughness))
        roughnessFactor = clamp(texture(u_Textures.RoughnessTexture, UV).r * u_Attributes.RoughnessFactor, 1e-4, 1.0);

    if(HasTexture(TB_Metallic))
        metallicFactor = clamp(texture(u_Textures.MetallicTexture, UV).r * u_Attributes.MetallicFactor, 0.0, 1.0);

    if(HasTexture(TB_Opacity))
        opacity = clamp(texture(u_Textures.OpacityTexture, UV).r * u_Attributes.Opacity, 0.0, 1.0);

    // AO precedence: ORM > AO > 1
    float aoTex = HasTexture(TB_AO)  ? texture(u_Textures.AOTexture,  UV).r : 1.0;
    float aoOrm = HasTexture(TB_ORM) ? texture(u_Textures.ORMTexture, UV).r : 1.0;
    AO = HasTexture(TB_ORM) ? aoOrm : aoTex;

    // Clearcoat textures
    if(HasTexture(TB_Clearcoat))
        clearcoatFactor = clamp(texture(u_Textures.ClearcoatTexture, UV).r * u_Attributes.ClearcoatFactor, 0.0, 1.0);
    if(HasTexture(TB_ClearcoatR))
        clearcoatRoughnessFactor = clamp(texture(u_Textures.ClearcoatRoughnessTexture, UV).r * u_Attributes.ClearcoatRoughnessFactor, 0.0, 1.0);

    // Specular textures
    if(HasTexture(TB_Spec))
        specLevel *= texture(u_Textures.SpecularTexture, UV).r;
    if(HasTexture(TB_SpecColor))
        specularColor *= texture(u_Textures.SpecularColorTexture, UV).rgb;

    // Sheen textures
    if(HasTexture(TB_SheenColor))
        sheenColor *= texture(u_Textures.SheenColorTexture, UV).rgb;
    if(HasTexture(TB_SheenR))
        sheenRoughnessFactor = clamp(texture(u_Textures.SheenRoughnessTexture, UV).r * u_Attributes.SheenRoughnessFactor, 0.0, 1.0);

    // Transmission textures
    if(HasTexture(TB_Trans))
        transmissionFactor = clamp(texture(u_Textures.TransmissionTexture, UV).r * u_Attributes.TransmissionFactor, 0.0, 1.0);
    if(HasTexture(TB_Thick))
        thicknessFactor = clamp(texture(u_Textures.ThicknessTexture, UV).r * u_Attributes.ThicknessFactor, 0.0, 1.0);

    // -------- F0 (metal/roughness with IOR + specular controls) --------
    float baseF0 = pow((IOR - 1.0) / (IOR + 1.0), 2.0);
    vec3  F0_dielectric = vec3(baseF0) * specLevel * specularColor;
    vec3  F0 = mix(F0_dielectric, baseColor, metallicFactor);

    // -------- Direct lighting (single sun) --------
    vec3  L     = normalize(-u_SunLight.Direction);
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float D = DistributionGGX(N, H, roughnessFactor);
    float G = GeometrySmith(N, V, L, roughnessFactor);
    vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicFactor); // conserve diffuse energy

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, EPS);
    vec3 Lo       = (kD * baseColor / PI + specular) * (u_SunLight.Color * u_SunLight.Intensity) * NdotL;

    // -------- IBL --------
    vec3 R = reflect(-V, N);

    float maxMipLevel    = max(u_IBLMipLevels, 1.0);
    float lod         = clamp(roughnessFactor * (maxMipLevel - 1.0), 0.0, maxMipLevel - 1.0);

    vec3 irradiance   = texture(u_IrradianceTexture, N).rgb * u_IBLIntensity_Diffuse;
    vec3 prefiltered  = textureLod(u_PrefilteredTexture, R, lod).rgb * u_IBLIntensity_Specular;
    vec2 brdf         = texture(u_BRDFLUTTexture, vec2(NdotV, roughnessFactor)).rg;

    vec3 F_ibl   = FresnelSchlickRoughness(NdotV, F0, roughnessFactor);
    vec3 kS_ibl  = F_ibl;
    vec3 kD_ibl  = (vec3(1.0) - kS_ibl) * (1.0 - metallicFactor);

    vec3 diffuseIBL  = irradiance * baseColor / PI;
    vec3 specularIBL = prefiltered * (F_ibl * brdf.x + brdf.y);

    vec3 ambient = (kD_ibl * diffuseIBL + specularIBL) * AO;

    // -------- Clearcoat (extra dielectric specular layer) --------
    if(clearcoatFactor > 0.0)
    {
        vec3  F0C = vec3(0.04); // typical urethane coat F0
        float DC  = DistributionGGX(N, H, clearcoatRoughnessFactor);
        float GC  = GeometrySmith(N, V, L, clearcoatRoughnessFactor);
        vec3  FC  = FresnelSchlick(max(dot(H, V), 0.0), F0C);
        vec3  specularCC = (DC * GC * FC) / max(4.0 * NdotV * NdotL, EPS);

        vec3 coatAttenuate = (vec3(1.0) - FC * clearcoatFactor);
        Lo      *= coatAttenuate;
        ambient *= coatAttenuate;

        Lo += specularCC * (u_SunLight.Color * u_SunLight.Intensity) * NdotL * clearcoatFactor;

        float lodC        = clearcoatRoughnessFactor * maxMipLevel;
        vec3 prefilteredC = textureLod(u_PrefilteredTexture, R, lodC).rgb * u_IBLIntensity_Specular;
        vec2 brdfC        = texture(u_BRDFLUTTexture, vec2(NdotV, clearcoatRoughnessFactor)).rg;
        vec3 specularIBLC = prefilteredC * (FresnelSchlickRoughness(NdotV, F0C, clearcoatRoughnessFactor) * brdfC.x + brdfC.y);
        ambient += specularIBLC * clearcoatFactor;
    }

    // -------- Sheen (cloth) --------
    if(HasTexture(TB_SheenColor) || length(sheenColor) > 1e-4)
    {
        if(length(sheenColor) > 1e-4)
        {
            float alpha = max(1e-4, sheenRoughnessFactor);
            float NdotH = max(dot(N, H), 0.0);
            float DS    = DistributionCharlie(NdotH, alpha);
            float VS    = VisibilitySheen(NdotL, NdotV);
            vec3  FS    = sheenColor;

            vec3 sheenSpecular = (DS * VS) * FS;
            Lo += sheenSpecular * NdotL;

            float lodS = clamp(sheenRoughnessFactor, 0.0, 1.0) * maxMipLevel;
            vec3 PRES  = textureLod(u_PrefilteredTexture, R, lodS).rgb * u_IBLIntensity_Specular;
            ambient   += PRES * sheenColor * 0.25;
        }
    }

    // -------- Transmission (thin-surface) + Volume-ish (attenuation) --------
    if(transmissionFactor > 0.0 || HasTexture(TB_Trans))
    {
        float t = transmissionFactor;
        if(HasTexture(TB_Trans))
            t = clamp(texture(u_Textures.TransmissionTexture, UV).r * u_Attributes.TransmissionFactor, 0.0, 1.0);

        if(t > 0.0)
        {
            vec3 RTR   = RefractEnvironment(V, N, max(IOR, 1.0));
            float lodT = roughnessFactor * maxMipLevel;
            vec3 transmissionEnv = textureLod(u_PrefilteredTexture, RTR, lodT).rgb * u_IBLIntensity_Specular;

            float thick = thicknessFactor;
            if(HasTexture(TB_Thick))
                thick = clamp(texture(u_Textures.ThicknessTexture, UV).r * u_Attributes.ThicknessFactor, 0.0, 1.0);

            float dist = attenuationDistance * thick;
            vec3  ATTE = Attenuation(attenuationColor, attenuationDistance, dist);
            vec3  transmitted = transmissionEnv * baseColor * ATTE;

            float reflectWeight  = max(max(F0.r, max(F0.g, F0.b)), 0.04);
            float transmitWeight = (1.0 - reflectWeight) * t;

            // Mix into ambient (thin sheet approximation)
            ambient = mix(ambient, transmitted, transmitWeight);
            // keep opacity as true coverage only
        }
    }

    // -------- Emissive --------
    vec3 emissiveColor = vec3(0.0);
    if(HasTexture(TB_Emissive))
    {
        emissiveColor  = texture(u_Textures.EmissiveTexture, UV).rgb;
        emissiveColor *= u_IBLIntensity_Specular; // simple strength
    }

    vec3 finalColor = ambient + Lo + emissiveColor;
    FragColor = vec4(finalColor, opacity);
}





















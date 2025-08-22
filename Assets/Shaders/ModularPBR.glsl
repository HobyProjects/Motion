#type vertex
#version 460 core

layout (location = 0) in vec3   a_Positions;
layout (location = 1) in vec2   a_TextureCoords;
layout (location = 2) in vec3   a_Normals;
layout (location = 3) in vec3   a_Tangents;
layout (location = 4) in vec3   a_Bitangents;
layout (location = 5) in float  a_TangentSign;

layout (location = 0) out vec3 v_WorldPos;
layout (location = 1) out vec2 v_UV;
layout (location = 2) out mat3 v_TBN;

const int TB_BASE_COLOR   = 1 << 0;
const int TB_METALLIC     = 1 << 1;
const int TB_ROUGHNESS    = 1 << 2;
const int TB_NORMALS      = 1 << 3;
const int TB_AO           = 1 << 4;
const int TB_EMISSIVE     = 1 << 5;
const int TB_OPACITY      = 1 << 6;
const int TB_ORM          = 1 << 7;
const int TB_DISPLACEMENT = 1 << 8;

uniform int   u_TextureBitmask;

uniform struct 
{
    vec4  BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float OcclusionStrength;
    vec3  EmissiveFactor;
    float EmissiveStrength;

    float NormalScale;
    float NormalYFlip;     
    float DisplacementScale;
    float DisplacementBias;

    float OpacityFactor;
    float AlphaCutoff;     
    int   AlphaMode;   

} u_Attributes;

uniform mat4  u_ModelMatrix;
uniform mat4  u_ViewMatrix;
uniform mat4  u_ProjectionMatrix;
uniform mat3  u_NormalMatrix; 

uniform sampler2D u_DisplacementTexture;
uniform sampler2D u_NormalTexture;

bool Has(int bit) { return (u_TextureBitmask & bit) != 0; }

void main()
{
    v_UV = a_TextureCoords;

    // --- Build world-space TBN (Gram–Schmidt + handedness) ---
    mat3 Nrm = u_NormalMatrix;
    vec3 T   = normalize(Nrm * a_Tangents);
    vec3 B   = normalize(Nrm * a_Bitangents);
    vec3 N   = normalize(Nrm * a_Normals);

    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T) * a_TangentSign);

    v_TBN = mat3(T, B, N);

    // --- Base world position ---
    vec4 worldPos = u_ModelMatrix * vec4(a_Positions, 1.0);

    // --- Displacement along world-space normal / macro-normal ---
    if (Has(TB_DISPLACEMENT))
    {
        // Default direction is the geometric normal
        vec3 directionW = N;

        // If we have a normal map, use a coarse LOD to stabilize macro displacement direction
        if (Has(TB_NORMALS))
        {
            vec3 nTS = textureLod(u_NormalTexture, a_TextureCoords, 4.0).xyz * 2.0 - 1.0;
            if (u_Attributes.NormalYFlip > 0.5) nTS.y = -nTS.y;
            nTS.xy *= max(u_Attributes.NormalScale, 0.0);
            vec3 nW  = normalize(v_TBN * nTS);
            directionW = normalize(mix(N, nW, 0.6));
        }

        // Sample height (support both filtered and texel-fetch paths)
        float height = texture(u_DisplacementTexture, a_TextureCoords).r;
        float disp    = u_Attributes.DisplacementScale * (height + u_Attributes.DisplacementBias);
        worldPos.xyz += directionW * disp;
    }

    v_WorldPos  = worldPos.xyz;
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * worldPos;
}

#type fragment
#version 460 core

layout (location = 0) in vec3 v_WorldPos;
layout (location = 1) in vec2 v_UV;
layout (location = 2) in mat3 v_TBN;

layout (location = 0) out vec4 FragColor;

const int TB_BASE_COLOR   = 1 << 0;
const int TB_METALLIC     = 1 << 1;
const int TB_ROUGHNESS    = 1 << 2;
const int TB_NORMALS      = 1 << 3;
const int TB_AO           = 1 << 4;
const int TB_EMISSIVE     = 1 << 5;
const int TB_OPACITY      = 1 << 6;
const int TB_ORM          = 1 << 7;
const int TB_DISPLACEMENT = 1 << 8;

uniform int u_TextureBitmask;

uniform struct 
{
    vec4  BaseColorFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float OcclusionStrength;
    vec3  EmissiveFactor;
    float EmissiveStrength;

    float NormalScale;
    float NormalYFlip;     
    float DisplacementScale;
    float DisplacementBias;

    float OpacityFactor;
    float AlphaCutoff;     
    int   AlphaMode;   

} u_Attributes;

// Environment / lighting
uniform samplerCube     u_IrradianceTexture;
uniform samplerCube     u_PrefilteredTexture;
uniform sampler2D       u_BRDFLUTTexture;
uniform float           u_IBLIntensity_Diffuse;
uniform float           u_IBLIntensity_Specular;
uniform float           u_IBLMipLevels;
uniform vec3            u_CameraPosition;

struct DirectionalLight
{
    vec3 Direction; // direction *towards* the light source
    vec3 Color;
    float Intensity;
};

uniform DirectionalLight u_SunLight;

// Textures
uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_MetallicTexture;
uniform sampler2D u_RoughnessTexture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_OcclusionTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_OpacityTexture;
uniform sampler2D u_ORMTexture; // R=Occlusion, G=Roughness, B=Metallic

const float PI = 3.14159265359;

bool  HasTex(int bit)   { return (u_TextureBitmask & bit) != 0; }
float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2  saturate(vec2 v)  { return clamp(v, vec2(0.0), vec2(1.0)); }
vec3  saturate(vec3 v)  { return clamp(v, vec3(0.0), vec3(1.0)); }

vec3 F_Schlick(vec3 F0, float VoH)
{
    float m = 1.0 - VoH;
    float m5 = m*m*m*m*m;
    return F0 + (1.0 - F0) * m5;
}

float D_GGX(float NoH, float a)
{
    float a2 = a*a;
    float d  = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float V_SmithG1(float NoV, float a)
{
    // Schlick-GGX G1
    float k = ((a + 1.0) * (a + 1.0)) / 8.0;
    return NoV / (NoV * (1.0 - k) + k + 1e-7);
}

float V_SmithGGX(float NoV, float NoL, float a)
{
    // Visibility term that already includes the 1/(4*NoV*NoL) denominator
    float gV = V_SmithG1(NoV, a);
    float gL = V_SmithG1(NoL, a);
    return (gV * gL) / (4.0 * max(NoV, 1e-4) * max(NoL, 1e-4) + 1e-7);
}

void computeF0AndAlbedo(vec3 baseColor, float metallic, out vec3 F0, out vec3 kd)
{
    vec3 F0dielectric = vec3(0.04);
    F0  = mix(F0dielectric, baseColor, metallic);
    // Energy conservation approximation: reduce diffuse by specular at normal incidence
    kd  = (1.0 - metallic) * (1.0 - F0);
}

vec3 specularIBL(vec3 N, vec3 V, float roughness, vec3 F0)
{
    float NoV           = max(dot(N, V), 0.0);
    vec3  R             = reflect(-V, N);
    float mip           = roughness * u_IBLMipLevels;
    vec3  prefiltered   = textureLod(u_PrefilteredTexture, R, mip).rgb;
    vec2  brdf          = texture(u_BRDFLUTTexture, vec2(NoV, roughness)).rg;
    return prefiltered * (F0 * brdf.x + brdf.y);
}

vec3 diffuseIBL(vec3 N)
{
    return texture(u_IrradianceTexture, N).rgb;
}

float evalAlpha(vec4 baseSample)
{
    // Mode: 0=OPAQUE, 1=MASK, 2=BLEND
    int mode = u_Attributes.AlphaMode;
    if (mode == 0) return 1.0; // OPAQUE

    float a = baseSample.a * max(u_Attributes.OpacityFactor, 0.0);
    if (HasTex(TB_OPACITY)) a *= texture(u_OpacityTexture, v_UV).r;

    if (mode == 1)
    {
        // Alpha test (MASK)
        if (a < u_Attributes.AlphaCutoff) discard;
        return 1.0;
    }

    // BLEND
    return clamp(a, 0.0, 1.0);
}

void main()
{
    // View & basis
    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 T = normalize(v_TBN[0]);
    vec3 B = normalize(v_TBN[1]);
    vec3 N = normalize(v_TBN[2]);

    // Normal mapping (world space)
    vec3 Nn = N;
    if (HasTex(TB_NORMALS))
    {
        vec3 nTS = texture(u_NormalTexture, v_UV).xyz * 2.0 - 1.0;
        if (u_Attributes.NormalYFlip > 0.5) nTS.y = -nTS.y;
        nTS.xy *= max(u_Attributes.NormalScale, 0.0);
        Nn = normalize(v_TBN * nTS);
    }

    // Base color
    vec4 baseSample = vec4(1.0);
    if (HasTex(TB_BASE_COLOR)) baseSample = texture(u_BaseColorTexture, v_UV);
    vec3 baseColor = saturate(baseSample.rgb * u_Attributes.BaseColorFactor.rgb);

    // Alpha (handles discard for MASK)
    float alpha = evalAlpha(baseSample);

    // Metallic / Roughness / AO
    float metallic  = u_Attributes.MetallicFactor;
    float roughness = u_Attributes.RoughnessFactor;
    float ao        = 1.0;

    if (HasTex(TB_ORM))
    {
        vec3 orm   = texture(u_ORMTexture, v_UV).rgb;
        ao         = mix(1.0, orm.r, u_Attributes.OcclusionStrength);
        roughness  = orm.g;
        metallic   = orm.b;
    }
    else
    {
        if (HasTex(TB_METALLIC))  metallic  = texture(u_MetallicTexture,  v_UV).r;
        if (HasTex(TB_ROUGHNESS)) roughness = texture(u_RoughnessTexture, v_UV).r;
        if (HasTex(TB_AO))        ao        = mix(1.0, texture(u_OcclusionTexture, v_UV).r, u_Attributes.OcclusionStrength);
    }

    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = saturate(metallic);

    // Emissive
    vec3 emissive = vec3(0.0);
    if (HasTex(TB_EMISSIVE)) emissive = texture(u_EmissiveTexture, v_UV).rgb * u_Attributes.EmissiveFactor * u_Attributes.EmissiveStrength;

    // Lighting vectors
    vec3 L = normalize(-u_SunLight.Direction);
    vec3 H = normalize(V + L);

    float NoV = max(dot(Nn, V), 0.0);
    float NoL = max(dot(Nn, L), 0.0);
    float NoH = max(dot(Nn, H), 0.0);
    float VoH = max(dot(V,  H), 0.0);

    // BRDF
    vec3 F0, kd;
    computeF0AndAlbedo(baseColor, metallic, F0, kd);
    float a = roughness * roughness;

    float  D = D_GGX(NoH, a);
    float  G = V_SmithGGX(NoV, NoL, a); // already includes 1/(4*NoV*NoL)
    vec3   F = F_Schlick(F0, VoH);
    vec3 specBRDF = (D * G) * F;
    vec3 diffBRDF = kd * baseColor / PI;

    // Direct lighting (not AO-attenuated)
    vec3 direct = (diffBRDF + specBRDF) * (NoL * u_SunLight.Color * u_SunLight.Intensity);

    // IBL (AO attenuates ambient only)
    vec3 iblD = diffuseIBL(Nn) * diffBRDF * u_IBLIntensity_Diffuse;
    vec3 iblS = specularIBL(Nn, V, roughness, F0) * u_IBLIntensity_Specular;

    vec3 color = direct + (iblD + iblS) * ao + emissive;
    FragColor = vec4(saturate(color), alpha);
}

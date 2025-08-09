// =============================================================
// PBR.glsl — expanded maps + bitmask + IBL intensity
// =============================================================

#type vertex
#version 460 core

// Vertex attributes (match your BufferLayout)
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in float a_TangentSign;

// Per-draw uniforms (renderer sets these)
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

// Precompute a proper normal matrix on CPU (renderer does this)
uniform mat3 u_NormalMatrix;

out VS_OUT {
    vec3 WorldPos;
    vec2 UV;
    mat3 TBN;
} vs_out;

void main() {
    vec3 N = normalize(u_NormalMatrix * a_Normal);
    vec3 T = normalize(u_NormalMatrix * a_Tangent);
    vec3 B = a_TangentSign * normalize(cross(N, T));

    vs_out.TBN      = mat3(T, B, N);
    vs_out.WorldPos = vec3(u_Model * vec4(a_Position, 1.0));
    vs_out.UV       = a_TexCoord;

    gl_Position = u_Proj * u_View * vec4(vs_out.WorldPos, 1.0);
}

#type fragment
#version 460 core

in VS_OUT {
    vec3 WorldPos;
    vec2 UV;
    mat3 TBN;
} fs_in;

layout(location = 0) out vec4 FragColor;

// -------------------------------------------------------------
// Camera + sun light (renderer sets these every view change)
// -------------------------------------------------------------
uniform vec3 u_CameraWorldPos;

struct DirLight 
{
    vec3 direction; // world-space
    vec3 color;     // linear
    float intensity;
};

uniform DirLight u_Sun;

// -------------------------------------------------------------
// IBL (bindings match renderer: 0, 1, 2)
// -------------------------------------------------------------
layout(binding = 0) uniform samplerCube u_IrradianceMap;     // diffuse IBL
layout(binding = 1) uniform samplerCube u_PrefilteredEnvMap; // specular IBL (mipmapped)
layout(binding = 2) uniform sampler2D   u_BRDFLUT;           // preintegrated BRDF

uniform float u_IBLIntensity_Diffuse  = 1.0;
uniform float u_IBLIntensity_Specular = 1.0;

// -------------------------------------------------------------
// Material constants + texture set (bindings start at 8)
// -------------------------------------------------------------
struct PBRAttributes 
{
    vec3  BaseColor;
    float Metallic;
    float Roughness;
    float Opacity;
};

uniform PBRAttributes u_Material;

// Bitmask of available textures (must match renderer bits)
uniform int u_TexMask;

const int TB_BaseColor  = 1<<0;
const int TB_Metallic   = 1<<1;
const int TB_Roughness  = 1<<2;
const int TB_Normal     = 1<<3;
const int TB_AO         = 1<<4;
const int TB_Emissive   = 1<<5;
const int TB_Opacity    = 1<<6;
const int TB_ORM        = 1<<7;   // R=AO, G=Roughness, B=Metallic
const int TB_Clearcoat  = 1<<8;
const int TB_ClearcoatR = 1<<9;
const int TB_SpecColor  = 1<<10;
const int TB_Spec       = 1<<11;
const int TB_SheenColor = 1<<12;
const int TB_SheenR     = 1<<13;
const int TB_Trans      = 1<<14;
const int TB_Thick      = 1<<15;

// Material samplers (match SceneRenderer TEX_SLOTS::Base + offsets)
layout(binding= 8) uniform sampler2D u_BaseColorTex;
layout(binding= 9) uniform sampler2D u_MetallicTex;
layout(binding=10) uniform sampler2D u_RoughnessTex;
layout(binding=11) uniform sampler2D u_NormalTex;
layout(binding=12) uniform sampler2D u_AOTex;
layout(binding=13) uniform sampler2D u_EmissiveTex;
layout(binding=14) uniform sampler2D u_OpacityTex;
layout(binding=15) uniform sampler2D u_ORMTex;
layout(binding=16) uniform sampler2D u_ClearcoatTex;
layout(binding=17) uniform sampler2D u_ClearcoatRTex;
layout(binding=18) uniform sampler2D u_SpecularColorTex;
layout(binding=19) uniform sampler2D u_SpecularTex;
layout(binding=20) uniform sampler2D u_SheenColorTex;
layout(binding=21) uniform sampler2D u_SheenRTex;
layout(binding=22) uniform sampler2D u_TransmissionTex;
layout(binding=23) uniform sampler2D u_ThicknessTex;

// (Optional) legacy bools; harmless with bitmask, kept for backward-compat
uniform bool u_HasBaseColorTex, u_HasMetallicTex, u_HasRoughnessTex, u_HasNormalTex, u_HasAOTex, u_HasEmissiveTex, u_HasOpacityTex;
uniform bool u_HasORMTex, u_HasClearcoatTex, u_HasClearcoatRTex, u_HasSpecularColorTex, u_HasSpecularTex, u_HasSheenColorTex, u_HasSheenRTex, u_HasTransmissionTex, u_HasThicknessTex;

// -------------------------------------------------------------
// Helpers
// -------------------------------------------------------------
const float PI = 3.14159265359;

vec3 SRGBToLinear(vec3 c)
{ 
    return pow(c, vec3(2.2)); 
}

vec3 LinearToSRGB(vec3 c)
{
    return pow(clamp(c, 0.0, 100.0), vec3(1.0/2.2));
}

float DistributionGGX(vec3 N, vec3 H, float roughness) 
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 1e-5);
}

float G_SchlickGGX(float NdotV, float roughness) 
{
    float r = roughness + 1.0;
    float k = (r*r)/8.0;
    return NdotV / (NdotV*(1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) 
{
    float NdotV = max(dot(N,V), 0.0);
    float NdotL = max(dot(N,L), 0.0);
    float ggx2 = G_SchlickGGX(NdotV, roughness);
    float ggx1 = G_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) 
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Normal mapping
    vec3 N = normalize(fs_in.TBN[2]);
    bool hasNormal = ((u_TexMask & TB_Normal) != 0) || u_HasNormalTex;
    if (hasNormal) {
        vec3 n = texture(u_NormalTex, fs_in.UV).xyz * 2.0 - 1.0;
        vec3 T = normalize(fs_in.TBN[0]);
        vec3 B = normalize(fs_in.TBN[1]);
        N = normalize(mat3(T,B,normalize(N)) * n);
    }

    vec3 V = normalize(u_CameraWorldPos - fs_in.WorldPos);

    // Base color
    vec3 baseColor = u_Material.BaseColor;
    if (((u_TexMask & TB_BaseColor) != 0) || u_HasBaseColorTex)
        baseColor *= SRGBToLinear(texture(u_BaseColorTex, fs_in.UV).rgb);

    // Metallic/Roughness/AO (packed ORM preferred)
    float metallic  = clamp(u_Material.Metallic,  0.0, 1.0);
    float roughness = clamp(u_Material.Roughness, 0.04, 1.0);
    float ao = 1.0;

    if (((u_TexMask & TB_ORM) != 0) || u_HasORMTex) {
        vec3 orm = texture(u_ORMTex, fs_in.UV).rgb;
        ao        = orm.r;
        roughness = clamp(roughness * orm.g, 0.04, 1.0);
        metallic  = clamp(metallic  * orm.b, 0.0,  1.0);
    } else {
        if (((u_TexMask & TB_Metallic)  != 0) || u_HasMetallicTex)  metallic  = clamp(metallic  * texture(u_MetallicTex,  fs_in.UV).r, 0.0, 1.0);
        if (((u_TexMask & TB_Roughness) != 0) || u_HasRoughnessTex) roughness = clamp(roughness * texture(u_RoughnessTex, fs_in.UV).g, 0.04, 1.0);
        if (((u_TexMask & TB_AO)        != 0) || u_HasAOTex)        ao        = texture(u_AOTex,        fs_in.UV).r;
    }

    float opacity = u_Material.Opacity;
    if (((u_TexMask & TB_Opacity) != 0) || u_HasOpacityTex)
        opacity *= texture(u_OpacityTex, fs_in.UV).r;

    // Optional cutout
    // if (opacity < 0.33) discard;

    // Base F0 (metallic workflow)
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    // ===== Direct lighting (single directional) =====
    vec3 L = normalize(-u_Sun.direction);
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);

    float  D = DistributionGGX(N, H, roughness);
    float  G = GeometrySmith(N, V, L, roughness);
    vec3   F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  specular = (D * G * F) / max(4.0 * max(dot(N,V),0.0) * NdotL, 1e-5);
    vec3  kS = F;
    vec3  kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 Lo = (kD * baseColor / PI + specular) * (u_Sun.color * u_Sun.intensity) * NdotL;

    // ===== IBL =====
    vec3 R = reflect(-V, N);

    // Diffuse IBL (Lambertian)
    vec3 irradiance = texture(u_IrradianceMap, N).rgb;
    vec3 diffuseIBL = irradiance * baseColor;

    // Specular IBL
    // Choose a conservative max mip count if unknown at runtime
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 prefiltered = textureLod(u_PrefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(u_BRDFLUT, vec2(max(dot(N,V),0.0), roughness)).rg;
    vec3 specIBL = prefiltered * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuseIBL * u_IBLIntensity_Diffuse + specIBL * u_IBLIntensity_Specular) * ao;

    vec3 color = Lo + ambient;

    // ===== Extended features =====
    // Emissive
    if (((u_TexMask & TB_Emissive) != 0) || u_HasEmissiveTex)
        color += SRGBToLinear(texture(u_EmissiveTex, fs_in.UV).rgb);

    // Clearcoat (extra specular lobe, simple env-only version)
    if (((u_TexMask & TB_Clearcoat) != 0) || u_HasClearcoatTex) 
    {
        float cc  = texture(u_ClearcoatTex,  fs_in.UV).r;
        float ccr = (((u_TexMask & TB_ClearcoatR) != 0) || u_HasClearcoatRTex) ? texture(u_ClearcoatRTex, fs_in.UV).r : 0.25;
        vec3  ccF0 = vec3(0.04);
        vec3  ccF  = FresnelSchlick(max(dot(N,V),0.0), ccF0);
        vec3  ccSpec = textureLod(u_PrefilteredEnvMap, R, ccr * MAX_REFLECTION_LOD).rgb * (ccF * brdf.x + brdf.y);
        color = mix(color, color + ccSpec, clamp(cc, 0.0, 1.0));
    }

    // Sheen
    if (((u_TexMask & TB_SheenColor) != 0) || u_HasSheenColorTex) 
    {
        vec3 sheenC = SRGBToLinear(texture(u_SheenColorTex, fs_in.UV).rgb);
        float sheenR = (((u_TexMask & TB_SheenR) != 0) || u_HasSheenRTex) ? texture(u_SheenRTex, fs_in.UV).r : 0.5;
        vec3 sheen = sheenC * textureLod(u_PrefilteredEnvMap, R, sheenR * MAX_REFLECTION_LOD).rgb;
        color += sheen * 0.25;
    }

    // Specular workflow assist (specular color / scalar)
    if (((u_TexMask & TB_Spec) != 0) || u_HasSpecularTex) 
    {
        float spec = texture(u_SpecularTex, fs_in.UV).r;
        vec3  specC = (((u_TexMask & TB_SpecColor) != 0) || u_HasSpecularColorTex)
                      ? SRGBToLinear(texture(u_SpecularColorTex, fs_in.UV).rgb)
                      : vec3(spec);
        color += specC * 0.1;
    }

    // Transmission / Thickness (cheap thin look)
    if (((u_TexMask & TB_Trans) != 0) || u_HasTransmissionTex) 
    {
        float trans = texture(u_TransmissionTex, fs_in.UV).r;
        float thick = (((u_TexMask & TB_Thick) != 0) || u_HasThicknessTex) ? texture(u_ThicknessTex, fs_in.UV).r : 0.5;
        color = mix(color, color + baseColor * trans * thick, 0.25);
    }

    // Tonemap + gamma
    color = color / (color + vec3(1.0));
    FragColor = vec4(LinearToSRGB(color), opacity);
}

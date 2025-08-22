#type vertex
#version 460 core

layout (location = 0) in vec3 a_Positions;
layout (location = 1) in vec2 a_TextureCoords;
layout (location = 2) in vec3 a_Normals;
layout (location = 3) in vec3 a_Tangents;
layout (location = 4) in vec3 a_Bitangents; // present for parity; we’ll renormalize

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat3 u_NormalMatrix;   

const int TB_Displacement = 1 << 18;

uniform float           u_DisplacementScale;  
uniform float           u_DisplacementBias;  
uniform sampler2D       u_DisplacementTexture;
uniform int             u_DisplacementBitMask;

// VS outputs
out v_OUT
{
    vec3 WorldPosition;
    vec2 UVs;
    vec3 Normals;
    vec3 Tangents;
    vec3 Bitangents;
    
} VOUT;

bool HasTexture(int bitMask) { return (u_DisplacementBitMask & bitMask) != 0; }

void main()
{
    // ---- Fetch inputs ----
    vec3 P_obj = a_Positions;
    vec2 UV    = a_TextureCoords;

    // ---- Vertex displacement (object space) ----
    // Height comes from the displacement texture R channel in [0..1] range
    // Final offset = (height * scale + bias) along the object-space normal.
    float height = 0.0;
    if (HasTexture(TB_Displacement))
    {
        height = texture(u_DisplacementTexture, UV).r;
    }
    float disp = height * u_DisplacementScale + u_DisplacementBias;

    // Displace along the (normalized) object-space normal
    vec3 N_obj = normalize(a_Normals);
    P_obj += N_obj * disp;

    // ---- Build world-space values ----
    vec4 P_world4 = u_ModelMatrix * vec4(P_obj, 1.0);
    vec3 N_world  = normalize(u_NormalMatrix * N_obj);

    // Recompute/renormalize T/B in world space for a clean TBN
    vec3 T_world = normalize(u_NormalMatrix * a_Tangents);
    vec3 B_world = normalize(u_NormalMatrix * a_Bitangents);

    // Orthonormalize T against N to reduce artifacts after displacement
    T_world = normalize(T_world - dot(T_world, N_world) * N_world);
    // Rebuild B from N and T for a guaranteed right-handed basis
    B_world = normalize(cross(N_world, T_world));

    // ---- Pass to fragment stage ----
    VOUT.WorldPosition = P_world4.xyz;
    VOUT.UVs           = UV;
    VOUT.Normals       = N_world;
    VOUT.Tangents      = T_world;
    VOUT.Bitangents    = B_world;

    // ---- Final clip position ----
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * P_world4;
}


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

    float NormalScale;           // normalTexture.scale
    float OcclusionStrength;     // occlusionTexture.strength
    float EmissiveStrength;      // KHR_materials_emissive_strength
    vec3  EmissiveColor;          // KHR_materials_emissive_color
    float AlphaCutoff;           // alpha test cutoff
    int   AlphaMode;             // 0=Opaque, 1=Mask, 2=Blend
    float ClearcoatNormalScale;  // clearcoat normal scale

    // Anisotropy
    float AnisotropyStrength;   // [0..1], 0 = isotropic, 1 = strong anisotropy
    float AnisotropyRotation;   // radians, rotates T/B basis around N

    // Iridescence
    float IridescenceFactor;        // [0..1]
    float IridescenceIor;           // film IOR, ~1.3 is common
    float IridescenceThicknessMin;  // nm, e.g., 100
    float IridescenceThicknessMax;  // nm, e.g., 400
};

// Texture bitmask flags
const int TB_BaseColor      = 1 << 0;
const int TB_Metallic       = 1 << 1;
const int TB_Roughness      = 1 << 2;
const int TB_Normal         = 1 << 3;
const int TB_AO             = 1 << 4;
const int TB_Emissive       = 1 << 5;
const int TB_Opacity        = 1 << 6;
const int TB_ORM            = 1 << 7;

const int TB_Clearcoat      = 1 << 8;
const int TB_ClearcoatR     = 1 << 9;
const int TB_SpecColor      = 1 << 10;
const int TB_Spec           = 1 << 11;
const int TB_SheenColor     = 1 << 12;
const int TB_SheenR         = 1 << 13;
const int TB_Trans          = 1 << 14;
const int TB_Thick          = 1 << 15;
const int TB_ClearcoatN     = 1 << 16; // <-- NEW for clearcoat normal map
// 17 is reserved for TB_Displacement
const int TB_Anisotropy       = 1 << 18;
const int TB_Iridescence      = 1 << 19;
const int TB_IridescenceThick = 1 << 20;

// Core
uniform sampler2D u_BaseColorTexture;
uniform sampler2D u_AOTexture;
uniform sampler2D u_MetallicTexture;
uniform sampler2D u_RoughnessTexture;
uniform sampler2D u_NormalMapTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_OpacityTexture;
uniform sampler2D u_ORMTexture;
uniform sampler2D u_ClearcoatTexture;
uniform sampler2D u_ClearcoatRoughnessTexture;
uniform sampler2D u_ClearcoatNormalTexture; 
uniform sampler2D u_SpecularColorTexture;
uniform sampler2D u_SpecularTexture;
uniform sampler2D u_SheenColorTexture;
uniform sampler2D u_SheenRoughnessTexture;
uniform sampler2D u_TransmissionTexture;
uniform sampler2D u_ThicknessTexture;
uniform sampler2D u_AnisotropyTexture;
uniform sampler2D u_IridescenceTexture;
uniform sampler2D u_IridescenceThicknessTexture;

// Globals / uniforms
uniform SunLight     u_SunLight;
uniform vec3         u_CameraPosition;
uniform float        u_NormalYFlip; // >0.5 => flip Y

uniform samplerCube  u_IrradianceTexture;
uniform samplerCube  u_PrefilteredTexture;
uniform sampler2D    u_BRDFLUTTexture;
uniform float        u_IBLIntensity_Diffuse;
uniform float        u_IBLIntensity_Specular;
uniform float        u_IBLMipLevels;  // if <=0, default used

uniform Attributes   u_Attributes;
uniform int          u_DisplacementBitMask;

bool HasTexture(int bitMask) { return (u_DisplacementBitMask & bitMask) != 0; }

const float PI  = 3.14159265359;
const float EPS = 1e-5;

// Fresnel
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX NDF
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = max(roughness * roughness, 1e-4);
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2= NdotH * NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
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

// Normal mapping
vec3 ApplyNormalMaps(vec3 N, vec3 T, vec3 B, vec2 UVs)
{
    if(!HasTexture(TB_Normal)) return normalize(N);

    vec3 n = texture(u_NormalMapTexture, UVs).xyz * 2.0 - 1.0;
    if (u_NormalYFlip > 0.5) n.y = -n.y;
    n.xy *= max(u_Attributes.NormalScale, 0.0); // <-- NEW

    // Column-major TBN: T, B, N
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
    return normalize(TBN * n);
}

// Clearcoat normal mapping (separate map/scale)
vec3 ApplyClearcoatNormal(vec3 N, vec3 T, vec3 B, vec2 UVs)
{
    if(!HasTexture(TB_ClearcoatN)) return normalize(N);
    vec3 n = texture(u_ClearcoatNormalTexture, UVs).xyz * 2.0 - 1.0;
    if (u_NormalYFlip > 0.5) n.y = -n.y;
    n.xy *= max(u_Attributes.ClearcoatNormalScale, 0.0);
    mat3 TBN = mat3(normalize(T), normalize(B), normalize(N));
    return normalize(TBN * n);
}

// Simple Beer-Lambert absorption helper
vec3 BeerLambert(vec3 attColor, float attDist, float distance)
{
    vec3 sigma_a = -log(max(attColor, vec3(1e-4))) / max(attDist, 1e-4);
    return exp(-sigma_a * distance);
}

// Rotate T/B by angle around N (keeps right-handed basis)
void RotateTB(in vec3 N, inout vec3 T, inout vec3 B, float angle)
{
    float c = cos(angle), s = sin(angle);
    vec3 Tn = c * T + s * B;
    vec3 Bn = normalize(cross(N, Tn));
    T = normalize(Tn);
    B = Bn;
}

float DistributionGGX_Aniso(vec3 N, vec3 H, vec3 T, vec3 B, float ax, float ay)
{
    float NdotH = max(dot(N, H), 0.0);
    float TdotH = dot(T, H);
    float BdotH = dot(B, H);
    float denom = (TdotH*TdotH)/(ax*ax) + (BdotH*BdotH)/(ay*ay) + NdotH*NdotH;
    return 1.0 / max(PI * ax * ay * denom * denom, EPS);
}

float Lambda_Aniso(vec3 N, vec3 w, vec3 T, vec3 B, float ax, float ay)
{
    float NdotW = max(dot(N, w), 0.0);
    float TdotW = dot(T, w);
    float BdotW = dot(B, w);
    float tan2  = ((TdotW*TdotW)/(ax*ax) + (BdotW*BdotW)/(ay*ay)) / max(NdotW*NdotW, 1e-6);
    return 0.5 * (sqrt(1.0 + tan2) - 1.0);
}

float GeometrySmith_Aniso(vec3 N, vec3 V, vec3 L, vec3 T, vec3 B, float ax, float ay)
{
    float lambdaV = Lambda_Aniso(N, V, T, B, ax, ay);
    float lambdaL = Lambda_Aniso(N, L, T, B, ax, ay);
    return 1.0 / (1.0 + lambdaV + lambdaL);
}

// Approx thin-film spectral modulation using three wavelengths (nm)
vec3 ThinFilmColor(float NdotV, float thickness_nm, float iorFilm, float iorExt /*=1.0*/)
{
    // Snell: cos(theta_t) inside the film
    float eta = iorExt / max(iorFilm, 1e-4);
    float sin2_t = eta*eta * (1.0 - NdotV*NdotV);
    float cos_t  = sqrt(clamp(1.0 - sin2_t, 0.0, 1.0));

    // Phase shift: δ = 4π n d cosθ / λ
    // Wavelengths for R,G,B (nm)
    const vec3 lambda = vec3(650.0, 510.0, 435.0);
    vec3 phase = 4.0 * PI * iorFilm * thickness_nm * cos_t / lambda;

    // Map interference to a color-ish term
    // 0.5 + 0.5*cos(phase) gives nice bands; add slight sharpening
    vec3 c = 0.5 + 0.5 * cos(phase);
    c = pow(c, vec3(0.7)); // gentle contrast curve
    return clamp(c, 0.0, 1.0);
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
    float opacity          = clamp(u_Attributes.Opacity, 0.0, 1.0); // base opacity
    float AO               = 1.0;

    // Clearcoat
    float clearcoatFactor          = clamp(u_Attributes.ClearcoatFactor, 0.0, 1.0);
    float clearcoatRoughnessFactor = clamp(u_Attributes.ClearcoatRoughnessFactor, 0.0, 1.0);

    // Specular
    vec3  specularColor = u_Attributes.SpecularColor;
    float specularLevel = u_Attributes.SpecularLevel;

    // Sheen
    vec3  sheenColor     = u_Attributes.SheenColor;
    float sheenRoughness = clamp(u_Attributes.SheenRoughnessFactor, 1e-4, 1.0);

    // Transmission/Volume/IOR
    float transmissionFactor  = clamp(u_Attributes.TransmissionFactor, 0.0, 1.0);
    float thicknessFactor     = clamp(u_Attributes.ThicknessFactor, 0.0, 1.0);
    vec3  attenuationColor    = u_Attributes.AttenuationColor;
    float attenuationDistance = max(u_Attributes.AttenuationDistance, 1e-4);
    float IOR                 = (u_Attributes.IOR > 0.0) ? u_Attributes.IOR : 1.5;

    // -------- Texture sampling --------
    if(HasTexture(TB_BaseColor))
        baseColor = texture(u_BaseColorTexture, UV).rgb * baseColor;

    if(HasTexture(TB_ORM) && !HasTexture(TB_Roughness) && !HasTexture(TB_Metallic))
    {
        vec3 ORM = texture(u_ORMTexture, UV).rgb; // R=AO, G=Roughness, B=Metallic
        metallicFactor  = clamp(ORM.b * metallicFactor, 0.0, 1.0);
        roughnessFactor = clamp(ORM.g * roughnessFactor, 1e-4, 1.0);
        AO              = ORM.r;
    }

    if(HasTexture(TB_Roughness) && !HasTexture(TB_ORM))
        roughnessFactor = clamp(texture(u_RoughnessTexture, UV).r * u_Attributes.RoughnessFactor, 1e-4, 1.0);

    if(HasTexture(TB_Metallic) && !HasTexture(TB_ORM))
        metallicFactor = clamp(texture(u_MetallicTexture, UV).r * u_Attributes.MetallicFactor, 0.0, 1.0);

    if(HasTexture(TB_Opacity))
        opacity *= texture(u_OpacityTexture, UV).r;

    // AO precedence: ORM > AO > 1
    float aoTex = HasTexture(TB_AO)  ? texture(u_AOTexture,  UV).r : 1.0;
    float aoOrm = HasTexture(TB_ORM) ? texture(u_ORMTexture, UV).r : 1.0;
    AO = HasTexture(TB_ORM) ? aoOrm : aoTex;
    AO = mix(1.0, AO, clamp(u_Attributes.OcclusionStrength, 0.0, 1.0)); // <-- NEW

    // Clearcoat textures
    if(HasTexture(TB_Clearcoat))
        clearcoatFactor = clamp(texture(u_ClearcoatTexture, UV).r * u_Attributes.ClearcoatFactor, 0.0, 1.0);

    if(HasTexture(TB_ClearcoatR))
        clearcoatRoughnessFactor = clamp(texture(u_ClearcoatRoughnessTexture, UV).r * u_Attributes.ClearcoatRoughnessFactor, 0.0, 1.0);

    // Specular extension textures
    if(HasTexture(TB_SpecColor))
        specularColor = texture(u_SpecularColorTexture, UV).rgb * specularColor;

    if(HasTexture(TB_Spec))
        specularLevel = texture(u_SpecularTexture, UV).r * specularLevel;

    // Sheen extension textures
    if(HasTexture(TB_SheenColor))
        sheenColor = texture(u_SheenColorTexture, UV).rgb * sheenColor;

    if(HasTexture(TB_SheenR))
        sheenRoughness = clamp(texture(u_SheenRoughnessTexture, UV).r * sheenRoughness, 1e-4, 1.0);

    // Transmission / Volume textures
    if(HasTexture(TB_Trans))
        transmissionFactor = clamp(texture(u_TransmissionTexture, UV).r * transmissionFactor, 0.0, 1.0);

    if(HasTexture(TB_Thick))
        thicknessFactor = clamp(texture(u_ThicknessTexture, UV).r * thicknessFactor, 0.0, 1.0);

     // --- Anisotropy params ---
    float anisoStrength = clamp(u_Attributes.AnisotropyStrength, 0.0, 1.0);
    float anisoAngle    = u_Attributes.AnisotropyRotation; // radians

    if (HasTexture(TB_Anisotropy))
    {
        // Texture .rg in [0..1] → [-1..1] tangent-space direction
        vec2 rg = texture(u_AnisotropyTexture, UV).rg * 2.0 - 1.0;
        float len = length(rg);
        if (len > 1e-4)
        {
            anisoAngle += atan(rg.y, rg.x);           // add texture direction
            anisoStrength = clamp(anisoStrength * len, 0.0, 1.0); // boost by texture length
        }
    }

    // Rotate T/B basis by final angle around N
    vec3 T_aniso = FIN.Tangents;
    vec3 B_aniso = FIN.Bitangents;
    RotateTB(N, T_aniso, B_aniso, anisoAngle);

    // Convert isotropic roughness to anisotropic αx, αy (Disney-style mapping)
    float a         = max(roughnessFactor * roughnessFactor, 1e-4);
    float aspect    = sqrt(clamp(1.0 - 0.9 * anisoStrength, 0.01, 1.0));
    float ax        = a / aspect;
    float ay        = a * aspect;


    // Alpha mode handling: 0=Opaque,1=Mask,2=Blend
    if(u_Attributes.AlphaMode == 1 && opacity < u_Attributes.AlphaCutoff)
        discard;
    if(u_Attributes.AlphaMode == 0)
        opacity = 1.0;

    // -------- Lighting inputs --------
    vec3 L = normalize(-u_SunLight.Direction);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // F0 from metallic/specular (dielectric ~0.04)
    vec3 F0 = mix(specularColor * specularLevel, baseColor, metallicFactor);

    // Cook-Torrance BRDF
    bool anisoOn = anisoStrength > 1e-4;
    float D = anisoOn ? DistributionGGX_Aniso(N, H, T_aniso, B_aniso, ax, ay) : DistributionGGX(N, H, roughnessFactor);
    float G = anisoOn ? GeometrySmith_Aniso(N, V, L, T_aniso, B_aniso, ax, ay) : GeometrySmith(N, V, L, roughnessFactor);
    vec3  F = FresnelSchlick(HdotV, F0);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicFactor);

    vec3 numerator    = D * G * F;
    float denom       = max(4.0 * NdotV * NdotL, EPS);
    vec3 specularBRDF = numerator / denom;

    vec3 Lo = (kD * baseColor / PI + specularBRDF) * (u_SunLight.Color * u_SunLight.Intensity) * NdotL;

    // --- Iridescence params ---
    float iriFactor    = clamp(u_Attributes.IridescenceFactor, 0.0, 1.0);
    float iriIor       = (u_Attributes.IridescenceIor > 0.0) ? u_Attributes.IridescenceIor : 1.3;
    float iriThickMin  = max(u_Attributes.IridescenceThicknessMin, 0.0);
    float iriThickMax  = max(u_Attributes.IridescenceThicknessMax, iriThickMin);
    float iriMask      = 1.0;

    // Optional masks
    if (HasTexture(TB_Iridescence))
        iriFactor *= texture(u_IridescenceTexture, UV).r;

    float tLerp = 0.5;
    if (HasTexture(TB_IridescenceThick))
        tLerp = texture(u_IridescenceThicknessTexture, UV).r;

    float iriThickness = mix(iriThickMin, iriThickMax, tLerp);

    // Compute thin-film color and blend into Fresnel
    vec3 filmColor = ThinFilmColor(NdotV, iriThickness, iriIor, 1.0);
    vec3 F_schlick = FresnelSchlick(HdotV, F0);
    vec3 F_irid    = mix(F_schlick, filmColor, iriFactor);

    // For IBL use an iridescent-tinted F0 as well
    vec3 F0_ibl = mix(F0, filmColor, iriFactor);


    // -------- IBL --------
    float maxMipLevel = (u_IBLMipLevels > 0.0) ? u_IBLMipLevels : 9.0;

    vec3 R          = reflect(-V, N);
    vec3 irradiance = texture(u_IrradianceTexture, N).rgb * u_IBLIntensity_Diffuse;
    vec3 prefiltered= textureLod(u_PrefilteredTexture, R, roughnessFactor * maxMipLevel).rgb * u_IBLIntensity_Specular;
    vec2 brdf       = texture(u_BRDFLUTTexture, vec2(NdotV, roughnessFactor)).rg;

    vec3 F_ibl  = FresnelSchlickRoughness(NdotV, F0_ibl, roughnessFactor);
    vec3 kS_ibl = F_ibl;
    vec3 kD_ibl = (vec3(1.0) - kS_ibl) * (1.0 - metallicFactor);

    vec3 diffuseIBL  = irradiance * baseColor / PI;
    vec3 specularIBL = prefiltered * (F_ibl * brdf.x + brdf.y);

    vec3 ambient = (kD_ibl * diffuseIBL + specularIBL) * AO;

    // -------- Clearcoat (extra dielectric specular layer) --------
    if(clearcoatFactor > 0.0)
    {
        // Optional clearcoat normal (separate)
        vec3 Ncc     = ApplyClearcoatNormal(FIN.Normals, FIN.Tangents, FIN.Bitangents, UV);
        float NccdotV= max(dot(Ncc, V), 0.0);
        float NccdotL= max(dot(Ncc, L), 0.0);
        vec3  Hcc    = normalize(V + L);

        vec3  F0C = vec3(0.04); // typical urethane coat F0
        float DC  = DistributionGGX(Ncc, Hcc, clearcoatRoughnessFactor);
        float GC  = GeometrySmith(Ncc, V, L, clearcoatRoughnessFactor);
        vec3  FC  = FresnelSchlick(max(dot(Hcc, V), 0.0), F0C);
        vec3  specularCC = (DC * GC * FC) / max(4.0 * NccdotV * NccdotL, EPS);

        vec3 coatAttenuate = (vec3(1.0) - FC * clearcoatFactor);
        Lo      *= coatAttenuate;
        ambient *= coatAttenuate;

        Lo += specularCC * (u_SunLight.Color * u_SunLight.Intensity) * NccdotL * clearcoatFactor;

        float lodC        = clearcoatRoughnessFactor * maxMipLevel;
        vec3 RC           = reflect(-V, Ncc);
        vec3 prefilteredC = textureLod(u_PrefilteredTexture, RC, lodC).rgb * u_IBLIntensity_Specular;
        vec2 brdfC        = texture(u_BRDFLUTTexture, vec2(NccdotV, clearcoatRoughnessFactor)).rg;
        vec3 specularIBLC = prefilteredC * (FresnelSchlickRoughness(NccdotV, F0C, clearcoatRoughnessFactor) * brdfC.x + brdfC.y);
        ambient += specularIBLC * clearcoatFactor;
    }

    // -------- Sheen (Charlie + wrap visibility) --------
    if(any(greaterThan(sheenColor, vec3(0.0))) || HasTexture(TB_SheenColor) || HasTexture(TB_SheenR))
    {
        float alpha = max(sheenRoughness * sheenRoughness, 1e-4);
        float NdotH = max(dot(N, H), 0.0);

        float D_sheen = (2.0 + 1.0/alpha) * pow(sqrt(1.0 - NdotH*NdotH), 1.0/alpha) / (2.0 * PI);
        float V_sheen = 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV)); // simple wrap vis

        vec3  F_sheen = sheenColor;
        vec3  sheenBRDF = F_sheen * D_sheen * V_sheen;
        Lo += sheenBRDF * (u_SunLight.Color * u_SunLight.Intensity) * NdotL;
    }

    // -------- Transmission / Volume (very simplified) --------
    if(transmissionFactor > 0.0)
    {
        float viewThickness = thicknessFactor * 1.0; // could be thickness from geometry
        vec3 absorb = BeerLambert(attenuationColor, attenuationDistance, viewThickness);
        vec3 trans  = baseColor * absorb * transmissionFactor;

        ambient = mix(ambient, ambient * trans, transmissionFactor);
        Lo      = mix(Lo,      Lo      * trans, transmissionFactor);
    }

    // -------- Emissive --------
    vec3 emissiveColor = vec3(0.0);
    if(HasTexture(TB_Emissive))
    {
        emissiveColor  = texture(u_EmissiveTexture, UV).rgb * u_Attributes.EmissiveColor;
        emissiveColor *= u_Attributes.EmissiveStrength; // <-- NEW
    }

    vec3 finalColor = ambient + Lo + emissiveColor;
    FragColor = vec4(finalColor, opacity);
}
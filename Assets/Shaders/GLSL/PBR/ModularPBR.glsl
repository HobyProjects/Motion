#type vertex
#version 460 core

//[FEATURES_ENABLE_DISABLE]

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec4 aTangent;
layout (location = 4) in vec3 aBitangent;

layout (location = 0) out vec3 vWorldPos;
layout (location = 1) out vec2 vUV;
layout (location = 2) out vec3 vT;
layout (location = 3) out vec3 vB;
layout (location = 4) out vec3 vN;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uCamPos;
uniform mat4 uModel;
uniform mat3 uNormal;

void main()
{
    vec3 worldPos = (uModel * vec4(aPosition, 1.0)).xyz;

    vec3 N  = normalize(uNormal * aNormal);
    vec3 T  = normalize(uNormal * aTangent.xyz);
    vec3 B  = normalize(cross(N, T) * aTangent.w);

    vWorldPos  = worldPos;
    vUV        = aTexCoord;
    vT         = T;
    vB         = B;
    vN         = N;

    gl_Position = uProj * uView * vec4(worldPos, 1.0);
}

#type fragment
#version 460 core

//[FEATURES_ENABLE_DISABLE]

layout (location = 0) in vec3 vWorldPos;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vT;
layout (location = 3) in vec3 vB;
layout (location = 4) in vec3 vN;

layout (location = 0) out vec4 oColor;

struct DirectionalLight
{
    vec3 Direction;
    vec3 Color;
    float Intensity;
};

struct MaterialAttributes
{
    vec4  BaseColorFactor;  
    vec3  EmissiveColor;     
    float NormalScale;       
    float MetallicFactor;    
    float RoughnessFactor;   
    float AOFactor;          
    float EmissiveStrength;  
    float OpacityFactor;     
};

uniform vec3                uCamPos;
uniform DirectionalLight    uLight;
uniform MaterialAttributes  uMat;

#ifdef USE_BASECOLOR_MAP
uniform sampler2D uBaseColorMap;
#endif
#ifdef USE_NORMAL_MAP
uniform sampler2D uNormalMap;
#endif
#ifdef USE_ORM_MAP
uniform sampler2D uORM;
#endif
#ifdef USE_OCCLUSION_MAP
uniform sampler2D uOcclusionMap;
#endif
#ifdef USE_ROUGHNESS_MAP
uniform sampler2D uRoughnessMap;
#endif
#ifdef USE_METALLIC_MAP
uniform sampler2D uMetallicMap;
#endif
#ifdef USE_EMISSIVE_MAP
uniform sampler2D uEmissiveMap;
#endif

uniform samplerCube uIrradianceMap;  
uniform samplerCube uPrefilteredMap; 
uniform sampler2D   uBRDFLUTMap;

const float PI = 3.14159265358979323846;
#define saturate(x) clamp(x, 0.0, 1.0)

float safeNdot(vec3 a, vec3 b) { return saturate(dot(a, b)); }
float safeNdotPos(vec3 a, vec3 b) { return max(dot(a, b), 1e-4); } 

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;

    float NdotH  = safeNdot(N, H);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 1e-6);
}

float GeometrySchlickGGX_Direct(float NdotX, float roughness)
{
    float r = saturate(roughness);
    float k = (r + 1.0);
    k = (k * k) * 0.125; // (r+1)^2 / 8

    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySchlickGGX_IBL(float NdotX, float roughness)
{
    float r = saturate(roughness);
    float k = (r * r) * 0.5;

    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith_Direct(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = safeNdotPos(N, V);
    float NdotL = safeNdot(N, L);

    float gv = GeometrySchlickGGX_Direct(NdotV, roughness);
    float gl = GeometrySchlickGGX_Direct(NdotL, roughness);
    return gv * gl;
}

float GeometrySmith_IBL(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = safeNdotPos(N, V);
    float NdotL = safeNdot(N, L);

    float gv = GeometrySchlickGGX_IBL(NdotV, roughness);
    float gl = GeometrySchlickGGX_IBL(NdotL, roughness);
    return gv * gl;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    float f = pow(saturate(1.0 - cosTheta), 5.0);
    return F0 + (1.0 - F0) * f;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    vec3  oneMinusR = vec3(1.0 - saturate(roughness));
    vec3  F90Clamp  = max(oneMinusR, F0);
    float f         = pow(saturate(1.0 - cosTheta), 5.0);
    return F0 + (F90Clamp - F0) * f;
}

void main()
{
    vec4 baseSample = uMat.BaseColorFactor;
#ifdef USE_BASECOLOR_MAP
    baseSample *= texture(uBaseColorMap, vUV);
#endif

    vec3 albedo     = saturate(baseSample.rgb);
    float opacity   = saturate(baseSample.a * uMat.OpacityFactor);
    
    float metallic  = uMat.MetallicFactor;
    float roughness = uMat.RoughnessFactor;
    float ao        = uMat.AOFactor;

#ifdef USE_ORM_MAP
    vec3 orm   = texture(uORM, vUV).rgb;
    ao        *= orm.r;
    metallic  *= orm.b;
    roughness *= orm.g;
#else
  #ifdef USE_OCCLUSION_MAP
    ao       *= texture(uOcclusionMap, vUV).r;
  #endif
  #ifdef USE_ROUGHNESS_MAP
    roughness *= texture(uRoughnessMap, vUV).r;
  #endif
  #ifdef USE_METALLIC_MAP
    metallic  *= texture(uMetallicMap, vUV).r;
  #endif
#endif

    roughness   = clamp(roughness, 0.4, 1.0);
    metallic    = saturate(metallic);
    ao          = saturate(ao);

    //////////////////////////////////////////////////////////////

    vec3 T   = normalize(vT);
    vec3 B   = normalize(vB);
    vec3 N   = normalize(vN);
    mat3 TBN = mat3(T, B, N);

    float normalScaling = uMat.NormalScale;
    vec3 nTS            = vec3(0.0);

#ifdef USE_NORMAL_MAP
    vec3 n  = texture(uNormalMap, vUV).rgb * 2.0 - 1.0;
    n.xy   *= max(normalScaling, 0.0);
    n.z     = sqrt(max(0.0, 1.0 - dot(n.xy, n.xy)));
    nTS     = n;
#else
    vec3 flatTS = vec3(0.0, 0.0, 1.0);
    nTS         = mix(flatTS, flatTS, normalScaling);   
#endif

    N           = normalize(TBN * nTS);
    vec3 V      = normalize(uCamPos - vWorldPos);
    float NoV   = clamp(dot(N, V), 1e-4, 1.0);

    //////////////////////////////////////////////////////////////

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    //////////////////////////////////////////////////////////////

    vec3 Lo     = vec3(0.0);

    vec3 L      = normalize(-uLight.Direction);
    vec3 H      = normalize(V + L);
    float NoL   = clamp(dot(N, L), 0.0, 1.0);

    if (NoL > 0.0)
    {
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith_Direct(N, V, L, roughness);
        float VoH = clamp(dot(V, H), 0.0, 1.0);
        vec3  F   = fresnelSchlick(VoH, F0);

        float NoV   = max(dot(N, V), 1e-4);
        vec3  spec  = (NDF * G * F) / max(4.0 * NoV * NoL, 1e-4);

        vec3 kS = F;                               
        vec3 kD = (1.0 - kS) * (1.0 - metallic);    

        Lo += (kD * albedo / PI + spec) * (uLight.Color * uLight.Intensity) * NoL;
    }

    //////////////////////////////////////////////////////////////

    vec3 R      = normalize(reflect(-V, N));
    vec3 F_ibl  = fresnelSchlickRoughness(NoV, F0, roughness);

    int   levels  = textureQueryLevels(uPrefilteredMap);
    float maxLod  = float(max(levels - 1, 0));      
    float lod     = roughness * maxLod;

    vec3 irradiance  = texture(uIrradianceMap, N).rgb;
    vec3 prefiltered = textureLod(uPrefilteredMap, R, lod).rgb;
    vec2 brdf        = texture(uBRDFLUTMap, vec2(NoV, roughness)).rg;

    vec3 kS = F_ibl;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float specAO = ao;

    vec3 envLighting    = kD * (irradiance * albedo / PI) * ao + prefiltered * (F_ibl * brdf.x + brdf.y) * specAO;
    vec3 color          = envLighting + Lo;

    //////////////////////////////////////////////////////////////

    vec3 emissive = uMat.EmissiveColor * uMat.EmissiveStrength;

#ifdef USE_EMISSIVE_MAP
    emissive *= texture(uEmissiveMap, vUV).rgb;
#endif

    color += emissive;

    //////////////////////////////////////////////////////////////

    oColor = vec4(color, opacity);
}
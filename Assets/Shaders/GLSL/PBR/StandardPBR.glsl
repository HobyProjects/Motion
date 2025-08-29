#type vertex
#version 460 core

//[FEATURES_ENABLE_DISABLE]

// --- Vertex Inputs ---
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec4 aTangent;   // .w = handedness (+1/-1)
layout (location = 4) in vec3 aBitangent; // optional; we'll rebuild B from N,T

// --- Vertex Outputs ---
layout (location = 0) out vec3 vWorldPos;
layout (location = 1) out vec2 vUV;
layout (location = 2) out vec3 vNormalWS;
layout (location = 3) out vec3 vTangentWS;
layout (location = 4) out vec3 vBitangentWS;

// --- Interfaces ---
struct Camera { mat4 View; mat4 Proj; vec3 CameraPos; };
struct Model  { mat4 Model; mat3 NormalMatrix; };

uniform Camera uCamera;
uniform Model  uModel;

void main()
{
    vec3 worldPos = (uModel.Model * vec4(aPosition, 1.0)).xyz;

    // Transform N,T, rebuild B with handedness
    vec3 N = normalize(uModel.NormalMatrix * aNormal);
    vec3 T = normalize(uModel.NormalMatrix * aTangent.xyz);
    vec3 B = normalize(cross(N, T) * aTangent.w);

    vWorldPos    = worldPos;
    vUV          = aTexCoord;
    vNormalWS    = N;
    vTangentWS   = T;
    vBitangentWS = B;

    gl_Position = uCamera.Proj * uCamera.View * vec4(worldPos, 1.0);
}


#type fragment
#version 460 core

//[FEATURES_ENABLE_DISABLE]

layout (location = 0) in vec3 vWorldPos;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormalWS;
layout (location = 3) in vec3 vTangentWS;
layout (location = 4) in vec3 vBitangentWS;

layout (location = 0) out vec4 oColor;

// ---- Interfaces ----
struct Camera { mat4 View; mat4 Proj; vec3 CameraPos; };
struct Model  { mat4 Model; mat3 NormalMatrix; };
struct Sun    { vec3 SunDirection; vec3 SunColor; float SunIntensity; };

// Material factors you set from C++ as `uMat.*`
struct Material
{
    vec4  BaseColorFactor;   // default (1,1,1,1)
    vec3  EmissiveColor;     // default (0,0,0)
    float NormalScale;       // default 1
    float MetallicFactor;    // default 1 (or 0 if you want dielectric default)
    float RoughnessFactor;   // default 1
    float AOFactor;          // default 1
    float OpacityFactor;     // default 1
    float EmissiveStrength;  // default 1
    float AlphaCutoff;       // default 0.5 for mask mode
};

uniform Camera   uCamera;
uniform Model    uModel;
uniform Sun      uSun;
uniform Material uMat;

// Samplers (samplers must remain standalone uniforms)
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
#ifdef USE_OPACITY_MAP
uniform sampler2D uOpacityMap;
#endif

// IBL inputs
uniform samplerCube uIrradiance;   // diffuse irradiance
uniform samplerCube uPrefiltered;  // specular env (mip chain)
uniform sampler2D   uBRDFLUT;      // split-sum BRDF LUT (RG)

// ---------------- Helpers ----------------
const float PI = 3.14159265358979323846;

float  saturate(float x){ return clamp(x, 0.0, 1.0); }
vec3   saturate(vec3  x){ return clamp(x, 0.0, 1.0); }


// GGX / Smith / Schlick
float D_GGX(float NoH, float a)
{
    float a2 = a * a;
    float d  = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}

float G1_Schlick(float NoV, float k)
{
    return NoV / (NoV * (1.0 - k) + k);
}

float V_SmithGGX_Schlick(float NoV, float NoL, float a)
{
    float k = (a + 1.0);
    k = (k * k) / 8.0;
    return G1_Schlick(NoV, k) * G1_Schlick(NoL, k);
}

vec3 F_Schlick(vec3 F0, float VoH)
{
    float f = pow(1.0 - VoH, 5.0);
    return F0 + (1.0 - F0) * f;
}

// ---- Sample helpers with fallbacks ----
vec4 SampleBaseColor(vec2 uv)
{
    vec4 c = uMat.BaseColorFactor;
#ifdef USE_BASECOLOR_MAP
    c *= texture(uBaseColorMap, uv);
#endif
    return c;
}

float SampleOpacity(vec2 uv)
{
    float a = uMat.OpacityFactor;
#ifdef USE_OPACITY_MAP
    a *= texture(uOpacityMap, uv).r;
#endif
    return saturate(a);
}

vec3 SampleEmissive(vec2 uv)
{
    vec3 e = uMat.EmissiveColor;
#ifdef USE_EMISSIVE_MAP
    e *= texture(uEmissiveMap, uv).rgb;
#endif
    return e * uMat.EmissiveStrength;
}

void SampleORM(vec2 uv, out float occlusion, out float roughness, out float metallic)
{
    occlusion = uMat.AOFactor;
    roughness = uMat.RoughnessFactor;
    metallic  = uMat.MetallicFactor;

#ifdef USE_ORM_MAP
    vec3 orm = texture(uORM, uv).rgb;             // R=Occlusion, G=Roughness, B=Metallic
    occlusion *= orm.r;
    roughness *= orm.g;
    metallic  *= orm.b;
#else
  #ifdef USE_OCCLUSION_MAP
    occlusion *= texture(uOcclusionMap, uv).r;
  #endif
  #ifdef USE_ROUGHNESS_MAP
    roughness *= texture(uRoughnessMap, uv).g;
  #endif
  #ifdef USE_METALLIC_MAP
    metallic  *= texture(uMetallicMap,  uv).b;
  #endif
#endif

    roughness = clamp(roughness, 0.04, 1.0); // avoid 0 for stability
    metallic  = saturate(metallic);
    occlusion = saturate(occlusion);
}

#ifdef USE_NORMAL_MAP
vec3 UnpackNormalTS(vec3 n)
{
    // Map [0,1] -> [-1,1]
    n = n * 2.0 - 1.0;
    n.xy *= max(uMat.NormalScale, 0.0);
    return normalize(n);
}

mat3 MakeTBN(vec3 T, vec3 B, vec3 N)
{
    // Orthonormalize (Gram-Schmidt)
    T = normalize(T - N * dot(N, T));
    B = normalize(cross(N, T));
    N = normalize(N);
    return mat3(T, B, N);
}
#endif

vec3 SampleNormalWS(vec2 uv, vec3 Nws, vec3 Tws, vec3 Bws)
{
#ifdef USE_NORMAL_MAP
    vec3 nTS = UnpackNormalTS(texture(uNormalMap, uv).rgb);
    mat3 TBN = MakeTBN(normalize(Tws), normalize(Bws), normalize(Nws));
    return normalize(TBN * nTS);
#else
    return normalize(Nws);
#endif
}



void main()
{
    // ----- Base color & factors -----
    vec4 baseColor = SampleBaseColor(vUV);
    vec3 albedo    = saturate(baseColor.rgb);

    // ----- Normal mapping -----
    vec3 N   = SampleNormalWS(vUV, vNormalWS, vTangentWS, vBitangentWS);
    vec3 V   = normalize(uCamera.CameraPos - vWorldPos);
    float NoV = max(dot(N, V), 1e-4);

    // ----- Material scalars -----
    float occlusion, roughness, metallic;
    SampleORM(vUV, occlusion, roughness, metallic);

    // ----- Fresnel base reflectance -----
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // ----- Optional directional (sun) contribution -----
    vec3 Lo = vec3(0.0);
    vec3 L  = normalize(-uSun.SunDirection);
    vec3 H  = normalize(V + L);

    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 1e-4);

    float alpha = max(roughness * roughness, 1e-4);
    float  D = D_GGX(NoH, alpha);
    float  G = V_SmithGGX_Schlick(NoV, NoL, alpha);
    vec3   F = F_Schlick(F0, VoH);

    vec3  kS = F;
    vec3  kD = (1.0 - kS) * (1.0 - metallic);

    float  denom = max(4.0 * NoV * NoL, 1e-4);
    vec3   spec  = (D * G * F) / denom;
    vec3   diff  = albedo / PI;

    vec3 sun  = (diff * kD + spec) * NoL * (uSun.SunColor * uSun.SunIntensity);
    Lo += sun;

    // ----- IBL -----
    vec3 R = reflect(-V, N);
    vec3 F_ibl = F_Schlick(F0, NoV);

    vec3  irradiance = texture(uIrradiance,  N).rgb;
    vec3  prefiltered= textureLod(uPrefiltered, R, roughness * 5.0).rgb;
    vec2  brdf       = texture(uBRDFLUT, vec2(NoV, roughness)).rg;

    vec3  diffuseIBL = irradiance * albedo;
    vec3  specIBL    = prefiltered * (F_ibl * brdf.x + brdf.y);

    vec3 color = occlusion * (kD * diffuseIBL / PI + specIBL); // + Lo

    // ----- Emissive -----
    color += SampleEmissive(vUV);

    // ----- Alpha pipeline -----
    float opacity = SampleOpacity(vUV);

#ifdef USE_ALPHA_MODE_MASK
    if (opacity < uMat.AlphaCutoff) discard;
    vec3 outColor = color;
    oColor = vec4(outColor, 1.0);

#elif defined(USE_ALPHA_MODE_BLEND)
    vec3 outColor = color;
  #ifdef USE_ALPHA_BLEND_PREMULTIPLIED
    oColor = vec4(outColor * opacity, opacity);
  #elif defined(USE_ALPHA_BLEND_ADDITIVE)
    oColor = vec4(outColor * opacity, 1.0);
  #else
    oColor = vec4(outColor, opacity);
  #endif

#else // OPAQUE (default)
    vec3 outColor = color;
    oColor = vec4(outColor, opacity);
#endif
}

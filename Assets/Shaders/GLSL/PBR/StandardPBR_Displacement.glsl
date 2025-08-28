#type vertex
#version 460 core

//[FEATURES_ENABLE_DISABLE]
// ^^^^^^^^^^^^^^^^^^^^^^^ DO NOT REMOVE (IT HELPS TO ADD #define AT RUNTIME)

layout (location = 0) in vec3  aPosition;
layout (location = 1) in vec2  aTexCoord;
layout (location = 2) in vec3  aNormal;
layout (location = 3) in vec4  aTangent;
layout (location = 4) in vec3  aBitangent;

layout(std140, binding = CAMERA_UBO_BINDING) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
};

layout(std140, binding = OBJECT_UBO_BINDING) uniform Object 
{
    mat4 uModel;
    mat3 uNormal;   // precomputed normal matrix (transpose(inverse(mat3(uModel))))
};

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vNormalWS;
layout(location = 3) out vec3 vTangentWS;
layout(location = 4) out vec3 vBitangentWS;

void main()
{
    vec4 wp     = uModel * vec4(aPosition, 1.0);
    mat3 nmat   = transpose(inverse(mat3(uModel)));

    vWorldPos   = wp.xyz;
    vUV         = aTexCoord;

    vec3 N      = normalize(nmat * aNormal);
    vNormalWS   = N;

    vec3 T      = normalize(nmat * aTangent.xyz);
    T           = normalize(T - N * dot(N, T));
    vec3 B      = normalize(cross(N, T)) * aTangent.w;

    vTangentWS      = T;
    vBitangentWS    = B;

    gl_Position     = uProj * uView * wp;
}

#type tessellation_control
#version 460 core

//[FEATURES_ENABLE_DISABLE]
// ^^^^^^^^^^^^^^^^^^^^^^^ DO NOT REMOVE (IT HELPS TO ADD #define AT RUNTIME)

layout (vertices = 3) out;

layout(std140, binding = CAMERA_UBO_BINDING) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
};

// VS outputs in:
layout(location = 0) in vec3 vWorldPos_VS[];
layout(location = 1) in vec2 vUV_VS[];
layout(location = 2) in vec3 vNormalWS_VS[];
layout(location = 3) in vec3 vTangentWS_VS[];
layout(location = 4) in vec3 vBitangentWS_VS[];

// TCS outputs forward to TES:
layout(location = 0) out vec3 vWorldPos_TCS[];
layout(location = 1) out vec2 vUV_TCS[];
layout(location = 2) out vec3 vNormalWS_TCS[];
layout(location = 3) out vec3 vTangentWS_TCS[];
layout(location = 4) out vec3 vBitangentWS_TCS[];

layout(std140, binding = MATERIAL_UBO_BINDING) uniform Material 
{
    vec4    uBaseColorFactor;
    float   uAlphaCutOff;
    float   uNormalScale;
    float   uMetallicFactor;
    float   uRoughnessFactor;
    float   uAOFactor;
    float   uOpacityFactor;
    float   uEmissiveStrength;
    float   uSpecularStrength;
    vec3    uEmissiveColor;
    float   uDisplacementScale;   
    float   uDisplacementBias;    
    float   uTessMin;             
    float   uTessMax;
    float   uPixPerEdge;         // target pixels per edge (higher = more tessellation)
    float   uLODNear;
    float   uLODFar;
};

// Project to NDC for a screen-space length estimate
float edgeScreenPixels(vec3 A, vec3 B) 
{
    vec4 a = uProj * uView * vec4(A, 1.0);
    vec4 b = uProj * uView * vec4(B, 1.0);
    a.xyz /= a.w; b.xyz /= b.w;

    // convert NDC delta to pixels (approx, assume 1080p; make a uniform if needed)
    float pix = length(a.xy - b.xy) * 0.5 * 1080.0;
    return pix;
}

float edgeTess(vec3 A, vec3 B) 
{
    float pix = edgeScreenPixels(A, B);
    float base = clamp(pix / max(uPixPerEdge, 1.0), 1.0, 64.0);

    // Distance-based attenuation (same for both verts → symmetric)
    float d = 0.5 * (distance(uCameraPos, A) + distance(uCameraPos, B));
    float lod = 1.0 - smoothstep(uLODNear, uLODFar, d);

    float t = base * (0.5 + 0.5 * lod);
    // Round to reduce crack risk across shared edges
    t = floor(t + 0.5);
    return clamp(t, uTessMin, uTessMax);
}

void main() 
{
    // Pass through per-vertex data
    vWorldPos_TCS[gl_InvocationID]      = vWorldPos_VS[gl_InvocationID];
    vUV_TCS[gl_InvocationID]            = vUV_VS[gl_InvocationID];
    vNormalWS_TCS[gl_InvocationID]      = vNormalWS_VS[gl_InvocationID];
    vTangentWS_TCS[gl_InvocationID]     = vTangentWS_VS[gl_InvocationID];
    vBitangentWS_TCS[gl_InvocationID]   = vBitangentWS_VS[gl_InvocationID];

    // Compute tess levels once per patch
    if (gl_InvocationID == 0) 
    {
        vec3 A      = vWorldPos_VS[0], B = vWorldPos_VS[1], C = vWorldPos_VS[2];
        float e0    = edgeTess(B, C);
        float e1    = edgeTess(C, A);
        float e2    = edgeTess(A, B);

        gl_TessLevelOuter[0] = e0;
        gl_TessLevelOuter[1] = e1;
        gl_TessLevelOuter[2] = e2;
        gl_TessLevelInner[0] = (e0 + e1 + e2) / 3.0;
    }
}

#type tessellation_evaluation
#version 460 core

//[FEATURES_ENABLE_DISABLE]
// ^^^^^^^^^^^^^^^^^^^^^^^ DO NOT REMOVE (IT HELPS TO ADD #define AT RUNTIME)

layout(triangles, equal_spacing, cw) in;

layout(std140, binding = CAMERA_UBO_BINDING) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
};

layout(binding = TEX_SLOT_DISPLACEMENT) uniform sampler2D uDisplacement; // R channel used

layout(std140, binding = MATERIAL_UBO_BINDING) uniform Material 
{
    vec4    uBaseColorFactor;
    float   uAlphaCutOff;
    float   uNormalScale;
    float   uMetallicFactor;
    float   uRoughnessFactor;
    float   uAOFactor;
    float   uOpacityFactor;
    float   uEmissiveStrength;
    float   uSpecularStrength;
    vec3    uEmissiveColor;
    float   uDisplacementScale;   
    float   uDisplacementBias;    
    float   uTessMin;             
    float   uTessMax;
    float   uPixPerEdge;         // target pixels per edge (higher = more tessellation)
    float   uLODNear;
    float   uLODFar;                         
};

// Inputs from TCS
layout(location = 0) in vec3 vWorldPos_TCS[];
layout(location = 1) in vec2 vUV_TCS[];
layout(location = 2) in vec3 vNormalWS_TCS[];
layout(location = 3) in vec3 vTangentWS_TCS[];
layout(location = 4) in vec3 vBitangentWS_TCS[];

// Outputs to FS (match standard VS outputs)
layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vNormalWS;     // geometric normal (pre-normalmap)
layout(location = 3) out vec3 vTangentWS;
layout(location = 4) out vec3 vBitangentWS;

void main() 
{
    vec3 b = vec3(gl_TessCoord.x, gl_TessCoord.y, gl_TessCoord.z);

    // Interpolate attributes
    vec3 P  = vWorldPos_TCS[0] * b.x + vWorldPos_TCS[1] * b.y + vWorldPos_TCS[2] * b.z;
    vec2 UV = vUV_TCS[0] * b.x + vUV_TCS[1] * b.y + vUV_TCS[2] * b.z;

    // Reconstruct geometric normal from the interp’d basis for stability
    vec3 N0 = normalize(vNormalWS_TCS[0]);
    vec3 N1 = normalize(vNormalWS_TCS[1]);
    vec3 N2 = normalize(vNormalWS_TCS[2]);
    vec3 N  = normalize(N0 * b.x + N1 * b.y + N2 * b.z);

    // Sample height and displace along the geometric normal
    float h     = texture(uDisplacement, UV).r; // [0,1]
    float disp  = h * uDisplacementScale + uDisplacementBias;
    vec3 Pdisp  = P + N * disp;

    // Interpolate T/B and re-orthogonalize to displaced normal
    vec3 T  = normalize(vTangentWS_TCS[0]*b.x + vTangentWS_TCS[1]*b.y + vTangentWS_TCS[2]*b.z);
    T       = normalize(T - N * dot(N, T));
    vec3 B  = normalize(cross(N, T)); // handedness already applied in VS

    // Outputs
    vWorldPos    = Pdisp;
    vUV          = UV;
    vNormalWS    = N;
    vTangentWS   = T;
    vBitangentWS = B;

    gl_Position = uProj * uView * vec4(Pdisp, 1.0);
}

#type fragment
#version 460 core

//[FEATURES_ENABLE_DISABLE]
// ^^^^^^^^^^^^^^^^^^^^^^^ DO NOT REMOVE (IT HELPS TO ADD #define AT RUNTIME)

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vNormalWS;
layout(location = 3) in vec3 vTangentWS;
layout(location = 4) in vec3 vBitangentWS;

layout(location = 0) out vec4 oColor;

// ---- UBOs ----
layout(std140, binding = CAMERA_UBO_BINDING) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
};

layout(std140, binding = MATERIAL_UBO_BINDING) uniform Material 
{
    vec4    uBaseColorFactor;
    float   uAlphaCutOff;
    float   uNormalScale;
    float   uMetallicFactor;
    float   uRoughnessFactor;
    float   uAOFactor;
    float   uOpacityFactor;
    float   uEmissiveStrength;
    float   uSpecularStrength;
    vec3    uEmissiveColor;
    float   uDisplacementScale;   
    float   uDisplacementBias;    
    float   uTessMin;             
    float   uTessMax;
    float   uPixPerEdge;         // target pixels per edge (higher = more tessellation)
    float   uLODNear;
    float   uLODFar;                         
};

layout(std140, binding = LIGHT_UBO_BINDING) uniform Sun 
{
    vec3  uSunDirection;   
    vec3  uSunColor;     
    float uSunIntensity; 
};

#ifdef USE_SH9
layout(std140, binding = SH9_UBO_BINDING) uniform SH9 {
    vec3 uSH[9]; // RGB coefficients
};
#endif

// ---- Samplers ----
// Color/data maps
layout(binding = TEX_SLOT_BASE_COLOR) uniform sampler2D uBaseColor;     // sRGB
layout(binding = TEX_SLOT_NORMAL) uniform sampler2D uNormalMap;         // linear
layout(binding = TEX_SLOT_METALLIC) uniform sampler2D uMetallic;        // linear (R)
layout(binding = TEX_SLOT_ROUGHNESS) uniform sampler2D uRoughness;      // linear (R)
layout(binding = TEX_SLOT_AO) uniform sampler2D uAO;                    // linear (R)
layout(binding = TEX_SLOT_EMISSIVE) uniform sampler2D uEmissive;        // sRGB
layout(binding = TEX_SLOT_OPACITY) uniform sampler2D uOpacity;          // linear (R), optional

#ifdef USE_ORM_MAP
layout(binding = TEX_SLOT_ORM) uniform sampler2D uORM;                  // linear, R=AO G=Rough B=Metal
#endif

// Diffuse IBL source: either SH9 (no sampler) or irradiance cubemap
#ifndef USE_SH9
layout(binding = TEX_SLOT_ENVIRONMENT_IRRADIANCE) uniform samplerCube uIrradiance;   // RGB16F
#endif

// Specular IBL (always available)
layout(binding = TEX_SLOT_ENVIRONMENT_PREFILTERED)  uniform samplerCube     uPrefiltered;   // RGB16F
layout(binding = TEX_SLOT_ENVIRONMENT_BRDFLUT) uniform sampler2D            uBRDFLUT;       // RG16F

const float PI = 3.14159265359;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec2  saturate(vec2 v)  { return clamp(v, vec2(0.0), vec2(1.0)); }
vec3  saturate(vec3 v)  { return clamp(v, vec3(0.0), vec3(1.0)); }

vec3 srgbToLinear(vec3 c)
{
    bvec3 m   = lessThanEqual(c, vec3(0.04045));
    vec3 low  = c / 12.92;
    vec3 high = pow((c + 0.055) / 1.055, vec3(2.4));
    return mix(high, low, vec3(m));
}

vec3 normalFromMap(vec2 uv, float strength)
{
    vec3 n = texture(uNormalMap, uv).xyz * 2.0 - 1.0; // TS
    n.xy *= strength;
    n = normalize(n);

    vec3 N  = normalize(vNormalWS);
    vec3 T  = normalize(vTangentWS);
    T       = normalize(T - N * dot(N, T));
    vec3 Bx = normalize(cross(N, T));

    // Recover handedness from provided bitangent
    float hand = (dot(Bx, normalize(vBitangentWS)) < 0.0) ? -1.0 : 1.0;
    vec3  B    = Bx * hand;

    return normalize(mat3(T, B, N) * n); // to world
}

// GGX helpers
float D_GGX(float NoH, float a)
{
    float a2 = a * a;
    float d  = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float G_SchlickGGX(float NdotX, float k) { return NdotX / (NdotX * (1.0 - k) + k); }

float G_Smith(float NoV, float NoL, float a)
{
    float k = (a + 1.0);
    k = (k * k) / 8.0;
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 Fresnel_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Specular IBL (split-sum): prefiltered env + BRDF LUT
vec3 SpecularIBL(vec3 N, vec3 V, float roughness, vec3 F0)
{
    float NoV         = max(dot(N, V), 0.0);
    vec3  R           = reflect(-V, N);
    float mipMax      = float(textureQueryLevels(uPrefiltered) - 1);
    float mip         = mipMax * roughness;
    vec3  prefiltered = textureLod(uPrefiltered, R, mip).rgb;
    vec2  brdf        = texture(uBRDFLUT, vec2(NoV, roughness)).rg;
    return prefiltered * (F0 * brdf.x + brdf.y);
}

#ifndef USE_SH9
vec3 DiffuseIBL(vec3 N)
{
    return texture(uIrradiance, N).rgb; // irradiance already integrated over hemisphere
}
#endif

#ifdef USE_SH9
vec3 evaluateSH9(vec3 n)
{
    float x = n.x, y = n.y, z = n.z;
    float b0 =  0.282095;
    float b1 = -0.488603 * y;
    float b2 =  0.488603 * z;
    float b3 = -0.488603 * x;
    float b4 =  1.092548 * x * y;
    float b5 = -1.092548 * y * z;
    float b6 =  0.315392 * (3.0 * z * z - 1.0);
    float b7 = -1.092548 * x * z;
    float b8 =  0.546274 * (x * x - y * y);

    return uSH[0]*b0 + uSH[1]*b1 + uSH[2]*b2 + uSH[3]*b3 + uSH[4]*b4 +
           uSH[5]*b5 + uSH[6]*b6 + uSH[7]*b7 + uSH[8]*b8;
}
#endif

float EvalAlpha(vec4 baseSample)
{
    // Combine tex alpha, material baseColor alpha, and opacity factor
    float opacity = clamp(baseSample.a * uBaseColorFactor.a * max(uOpacityFactor, 0.0), 0.0, 1.0);

#ifdef USE_OPACITY_MAP
    opacity *= texture(uOpacity, vUV).r;
#endif

#ifdef USE_ALPHA_MODE_OPAQUE
    return 1.0; 
#endif

#ifdef USE_ALPHA_MODE_MASK
    if (opacity < uAlphaCutOff) discard;
    return 1.0;
#endif

#ifdef USE_ALPHA_MODE_BLEND
    return opacity;
#endif
}

void main()
{
    // --- Base color & opacity ---
    vec4 baseSample = texture(uBaseColor, vUV);                // sRGB
    vec3 baseColor  = saturate(srgbToLinear(baseSample.rgb) * uBaseColorFactor.rgb);
    float opacity   = EvalAlpha(baseSample);

    // --- Shading frame ---
    vec3 N    = normalFromMap(vUV, uNormalScale);
    vec3 V    = normalize(uCameraPos - vWorldPos);
    float NoV = max(dot(N, V), 0.0);

    // --- Material scalars (ORM or separate) ---
#ifdef USE_ORM_MAP
    vec3 orm        = texture(uORM, vUV).rgb;
    float ao        = clamp(orm.r * uAOFactor,        0.0, 1.0);
    float roughness = clamp(orm.g * uRoughnessFactor, 0.04, 1.0);
    float metallic  = clamp(orm.b * uMetallicFactor,  0.0, 1.0);
#else
    float ao        = clamp(texture(uAO,        vUV).r * uAOFactor,        0.0, 1.0);
    float roughness = clamp(texture(uRoughness, vUV).r * uRoughnessFactor, 0.04, 1.0);
    float metallic  = clamp(texture(uMetallic,  vUV).r * uMetallicFactor,  0.0, 1.0);
#endif

    // --- Sun (directional) vectors ---
    vec3 L  = normalize(-uSunDirection);       // light direction from surface to sun
    vec3 H  = normalize(V + L);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float VoH = max(dot(V, H), 0.0);

    // --- BRDF parameters ---
    vec3  F0  = mix(vec3(0.04), baseColor, metallic);
    float a   = roughness * roughness;

    // Microfacet terms
    float D = D_GGX(NoH, a);
    float G = G_Smith(NoV, NoL, a);
    vec3  F = Fresnel_Schlick(VoH, F0);

    // Energy-consistent diffuse: (1 - metallic) * (1 - F)
    vec3 kd = (1.0 - metallic) * (1.0 - F);

    // Specular BRDF (with denominator), optional strength scale
    vec3 specularBRDF = (D * G) * F / max(4.0 * NoV * NoL, 1e-5);
    specularBRDF     *= max(uSpecularStrength, 0.0);

    // Diffuse BRDF (Lambert)
    vec3 diffuseBRDF  = kd * baseColor * (1.0 / PI);

    // --- Direct lighting (Sun) ---
    vec3 sunRadiance = uSunColor * uSunIntensity; // linear HDR
    vec3 direct = (diffuseBRDF + specularBRDF) * sunRadiance * NoL;

    // --- Ambient / IBL ---
#ifdef USE_SH9
    vec3 irradiance = evaluateSH9(N);
#else
    vec3 irradiance = DiffuseIBL(N);
#endif
    // Diffuse IBL uses irradiance * diffuse color (no /PI; irradiance already integrated)
    vec3 diffuseIBL  = irradiance * (kd * baseColor);

    // Specular IBL uses prefiltered env + BRDF LUT (split-sum)
    vec3 specularIBL = SpecularIBL(N, V, roughness, F0);

    vec3 ambient = (diffuseIBL + specularIBL) * ao;

    vec3 emissiveTex = srgbToLinear(texture(uEmissive, vUV).rgb);
    vec3 emissive    = emissiveTex * uEmissiveColor * uEmissiveStrength;

    vec3 color = direct + ambient + emissive;

#ifdef USE_ALPHA_MODE_BLEND
    #ifdef USE_ALPHA_BLEND_PREMULTIPLIED
        oColor = vec4(color * opacity, opacity); 
    #elif defined(USE_ALPHA_BLEND_ADDITIVE)
        oColor = vec4(color * opacity, 0.0);
    #elif defined(USE_ALPHA_BLEND)
        oColor = vec4(color, opacity);
    #else
        oColor = vec4(color, opacity);         
    #endif
#else
    
    oColor = vec4(color, 1.0);
#endif
}
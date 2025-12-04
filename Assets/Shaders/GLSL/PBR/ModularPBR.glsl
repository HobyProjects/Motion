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
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPosition.xyz;

    // Transform normal, tangent, and bitangent to world space
    vec3 N = normalize(uNormal * aNormal);
    vec3 T = normalize(uNormal * aTangent.xyz);
    
    // Re-orthogonalize T with respect to N (Gram-Schmidt process)
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent
    vec3 B = cross(N, T) * aTangent.w;

    vT = T;
    vB = B;
    vN = N;
    vUV = aTexCoord;

    gl_Position = uProj * uView * worldPosition;
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

// ============================================================================
// STRUCTURES
// ============================================================================

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

// ============================================================================
// UNIFORMS
// ============================================================================

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

// ============================================================================
// CONSTANTS
// ============================================================================

const float PI = 3.14159265359;
const float EPSILON = 0.00001;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }

// ============================================================================
// PBR CORE FUNCTIONS
// ============================================================================

// Normal Distribution Function: GGX/Trowbridge-Reitz
float D_GGX(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    
    float numerator = alpha2;
    float denominator = NdotH2 * (alpha2 - 1.0) + 1.0;
    denominator = PI * denominator * denominator;
    
    return numerator / max(denominator, EPSILON);
}

// Geometry Function: Smith's Schlick-GGX
float G_SchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    
    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;
    
    return numerator / max(denominator, EPSILON);
}

// Smith's method
float G_Smith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = G_SchlickGGX(NdotV, roughness);
    float ggx1 = G_SchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel-Schlick with roughness for ambient/IBL
vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// ============================================================================
// REALISTIC AMBIENT LIGHTING
// ============================================================================

// Multi-bounce diffuse approximation
// Accounts for light bouncing multiple times within rough surfaces
vec3 getMultiScatterDiffuse(vec3 albedo, float NdotV, float roughness)
{
    // Approximation of additional energy from multiple bounces
    float multiScatter = 0.5 * roughness;
    return albedo * (1.0 + albedo * multiScatter);
}

// Improved ambient with directional influence
vec3 getRealisticAmbient(vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, float ao)
{
    // Sky dome approximation
    vec3 skyColorTop = vec3(0.5, 0.6, 0.8);      // Blue sky
    vec3 skyColorHorizon = vec3(0.8, 0.75, 0.7); // Warm horizon
    vec3 groundColor = vec3(0.3, 0.25, 0.2);     // Brown/earth ground
    
    // Vertical gradient for sky
    float skyFactor = saturate(N.y);
    vec3 skyContribution = mix(skyColorHorizon, skyColorTop, skyFactor * skyFactor);
    
    // Ground contribution (negative Y normals)
    float groundFactor = saturate(-N.y);
    vec3 ambientColor = mix(skyContribution, groundColor, groundFactor);
    
    // Ambient intensity
    float ambientIntensity = 0.15;
    
    // Fresnel for ambient (rough approximation of environment reflections)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F_SchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    // Energy conservation for ambient
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    
    // Diffuse ambient
    vec3 diffuseAmbient = kD * albedo * ambientColor * ambientIntensity;
    
    // Specular ambient (fake environment reflection based on view angle)
    float NdotV = max(dot(N, V), 0.0);
    vec3 specularAmbient = kS * ambientColor * pow(1.0 - roughness, 2.0) * 0.15;
    
    return (diffuseAmbient + specularAmbient) * ao;
}

// Soft subsurface scattering approximation for non-metals
vec3 getSubsurfaceScattering(vec3 L, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic)
{
    // Only apply to non-metallic surfaces
    if (metallic > 0.5) return vec3(0.0);
    
    // Back-scattering approximation
    float VdotL = dot(V, -L);
    float scatter = saturate(VdotL) * saturate(-dot(N, L) + 0.5);
    
    // More scattering for rougher surfaces
    float scatterStrength = roughness * 0.3;
    
    return albedo * scatter * scatterStrength * 0.5;
}

// ============================================================================
// IMPROVED TONE MAPPING
// ============================================================================

// Uncharted 2 tone mapping (more realistic than simple Reinhard)
vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 applyTonemap(vec3 color)
{
    float exposure = 1.0;
    color *= exposure;
    
    // Apply Uncharted 2 tonemap
    float W = 11.2; // White point
    vec3 curr = Uncharted2Tonemap(color);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    return curr * whiteScale;
}

// ============================================================================
// MATERIAL SAMPLING
// ============================================================================

vec3 getNormal()
{
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);
    
#ifdef USE_NORMAL_MAP
    tangentNormal = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
    tangentNormal.xy *= uMat.NormalScale;
    tangentNormal.z = sqrt(max(0.0, 1.0 - dot(tangentNormal.xy, tangentNormal.xy)));
#endif
    
    vec3 T = normalize(vT);
    vec3 B = normalize(vB);
    vec3 N = normalize(vN);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

// ============================================================================
// MAIN FRAGMENT SHADER
// ============================================================================

void main()
{
    // ========================================================================
    // MATERIAL PROPERTIES SAMPLING
    // ========================================================================
    
    vec4 baseColor = uMat.BaseColorFactor;
#ifdef USE_BASECOLOR_MAP
    baseColor *= texture(uBaseColorMap, vUV);
#endif
    
    vec3 albedo = baseColor.rgb;
    float alpha = baseColor.a * uMat.OpacityFactor;
    
    float metallic = uMat.MetallicFactor;
    float roughness = uMat.RoughnessFactor;
    float ao = uMat.AOFactor;
    
#ifdef USE_ORM_MAP
    vec3 orm = texture(uORM, vUV).rgb;
    ao *= orm.r;
    roughness *= orm.g;
    metallic *= orm.b;
#else
  #ifdef USE_OCCLUSION_MAP
    ao *= texture(uOcclusionMap, vUV).r;
  #endif
  #ifdef USE_ROUGHNESS_MAP
    roughness *= texture(uRoughnessMap, vUV).g;
  #endif
  #ifdef USE_METALLIC_MAP
    metallic *= texture(uMetallicMap, vUV).b;
  #endif
#endif
    
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = saturate(metallic);
    ao = saturate(ao);
    
    // ========================================================================
    // LIGHTING SETUP
    // ========================================================================
    
    vec3 N = getNormal();
    vec3 V = normalize(uCamPos - vWorldPos);
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // ========================================================================
    // DIRECT LIGHTING
    // ========================================================================
    
    vec3 Lo = vec3(0.0);
    
    vec3 L = normalize(-uLight.Direction);
    vec3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), EPSILON);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    
    if (NdotL > 0.0)
    {
        // Cook-Torrance BRDF
        float D = D_GGX(NdotH, roughness);
        float G = G_Smith(NdotV, NdotL, roughness);
        vec3 F = F_Schlick(HdotV, F0);
        
        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular = numerator / max(denominator, EPSILON);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 radiance = uLight.Color * uLight.Intensity;
        
        // Add diffuse and specular
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        
        // Add subtle subsurface scattering for realism
        vec3 sss = getSubsurfaceScattering(L, N, V, albedo, roughness, metallic);
        Lo += sss * radiance;
    }
    
    // ========================================================================
    // AMBIENT LIGHTING (Realistic)
    // ========================================================================
    
    vec3 ambient = getRealisticAmbient(N, V, albedo, roughness, metallic, ao);
    
    // ========================================================================
    // EMISSIVE
    // ========================================================================
    
    vec3 emissive = uMat.EmissiveColor * uMat.EmissiveStrength;
    
#ifdef USE_EMISSIVE_MAP
    emissive *= texture(uEmissiveMap, vUV).rgb;
#endif
    
    // ========================================================================
    // FINAL COLOR
    // ========================================================================
    
    vec3 color = ambient + Lo + emissive;
    
    // Improved tone mapping
    color = applyTonemap(color);
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    oColor = vec4(color, alpha);
}

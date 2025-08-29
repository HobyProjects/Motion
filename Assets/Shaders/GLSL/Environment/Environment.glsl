#type vertex
#version 460 core
layout(location=0) in vec3 a_Pos;

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_Dir;

void main()
{
    v_Dir       = a_Pos; 
    gl_Position = (u_Proj * u_View * vec4(a_Pos, 1.0)).xyww; 
}


#type fragment
#version 460 core

layout(location=0) out vec4 FragColor;

in vec3 v_Dir;

uniform samplerCube u_EnvironmentTexture;

// Controls
uniform float u_SkyIntensity  = 1.0;  // linear scale
uniform float u_SkyExposure   = 0.0;  // in stops; 1.0 = 2x
uniform float u_SkyGamma      = 2.2;  // display gamma
uniform float u_SkyMipLevel   = -1.0; // < 0 => automatic (base)
uniform int   u_Tonemap       = 0;    // 0 = none (linear), 1 = ACES, 2 = Reinhard

// Tonemap helpers
vec3 TonemapACES(vec3 x) {
    // Narkowicz ACES approximation
    const float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}
vec3 TonemapReinhard(vec3 x) {
    return x / (1.0 + x);
}

void main()
{
    vec3 dir = normalize(v_Dir);

    // Clamp explicit LOD to available chain
    int maxLod = textureQueryLevels(u_EnvironmentTexture) - 1;
    float lod = (u_SkyMipLevel >= 0.0)
        ? clamp(u_SkyMipLevel, 0.0, float(maxLod))
        : 0.0;

    vec3 c = (u_SkyMipLevel >= 0.0)
        ? textureLod(u_EnvironmentTexture, dir, lod).rgb
        : texture(u_EnvironmentTexture, dir).rgb;

    // Exposure in stops and intensity multiplier
    c *= exp2(clamp(u_SkyExposure, -20.0, 20.0));
    c *= max(u_SkyIntensity, 0.0);

    // Optional tonemap to display range
    if (u_Tonemap == 1)      c = TonemapACES(c);
    else if (u_Tonemap == 2) c = TonemapReinhard(c);

    // Output (apply display gamma if needed)
    float g = max(u_SkyGamma, 1e-4);
    c = pow(max(c, vec3(0.0)), vec3(1.0 / g));

    FragColor = vec4(c, 1.0);
}


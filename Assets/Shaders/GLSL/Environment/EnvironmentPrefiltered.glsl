#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos;

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_Dir;

void main()
{
    v_Dir       = a_Pos;
    gl_Position = (u_Proj * u_View * vec4(a_Pos,1.0)).xyww;
}

#type fragment
#version 460 core

in vec3 v_Dir;
layout(location=0) out vec4 FragColor;

uniform samplerCube u_EnvironmentTexture;  // original env cube (mipmapped)
uniform float       u_PrefilteredRoughness; // [0,1]

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i)/float(n), RadicalInverse_VdC(i));
}

// GGX importance sampling ---------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, float a, vec3 N)
{
    float phi  = 2.0 * PI * Xi.x;
    float cosT = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinT = sqrt(max(1.0 - cosT*cosT, 0.0));
    vec3 H     = vec3(cos(phi) * sinT, sin(phi) * sinT, cosT);

    vec3 up    = (abs(N.z) < 0.999) ? vec3(0,0,1) : vec3(1,0,0);
    vec3 T     = normalize(cross(up, N));
    vec3 B     = cross(N, T);
    return normalize(mat3(T,B,N) * H);
}

float DistributionGGX(float NoH, float a)
{
    float a2 = a * a;
    float d  = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

void main()
{
    vec3  N      = normalize(v_Dir);
    vec3  V      = N;
    float rough  = clamp(u_PrefilteredRoughness, 0.0, 1.0);
    float a      = max(rough * rough, 1e-4);

    const uint SAMPLES = 1024u;
    vec3 sum = vec3(0);
    float wsum = 0.0;

    for(uint i=0u; i<SAMPLES; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLES);
        vec3 H  = ImportanceSampleGGX(Xi, a, N);
        vec3 L  = normalize(2.0 * dot(V,H) * H - V);

        float NoL = max(dot(N,L), 0.0);
        float NoH = max(dot(N,H), 0.0);
        float VoH = max(dot(V,H), 0.0);

        if (NoL > 0.0)
        {
            float pdf  = (DistributionGGX(NoH, a) * NoH) / max(4.0 * VoH, 1e-4);
            float w    = max(VoH * NoL / max(pdf, 1e-6), 0.0);
            sum  += texture(u_EnvironmentTexture, L).rgb * w;
            wsum += w;
        }
    }

    vec3 prefiltered = sum / max(wsum, 1e-4);
    FragColor        = vec4(prefiltered, 1.0);
}

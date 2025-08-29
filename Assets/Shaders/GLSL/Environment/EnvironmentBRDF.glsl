#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos;
layout(location=1) in vec2 a_UV;

out vec2 v_UV;

void main()
{
    v_UV = a_UV;
    gl_Position = vec4(a_Pos, 1.0);
}

#type fragment
#version 460 core

in vec2 v_UV;
layout(location=0) out vec2 FragColor; // store BRDF LUT in RG

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

float G_Smith(float NoV, float NoL, float a)
{
    float k = (a * a) * 0.5;
    float Gv = NoV / (NoV * (1.0 - k) + k);
    float Gl = NoL / (NoL * (1.0 - k) + k);
    return Gv * Gl;
}

vec2 IntegrateBRDF(float NoV, float rough)
{
    vec3 V;
    V.x = sqrt(1.0 - NoV * NoV);
    V.y = 0.0;
    V.z = NoV;

    float a = max(rough * rough, 1e-4);

    const uint SAMPLES = 1024u;
    float A = 0.0;
    float B = 0.0;

    for (uint i = 0u; i < SAMPLES; ++i)
    {
        vec2 Xi  = Hammersley(i, SAMPLES);
        float phi = 2.0 * PI * Xi.x;
        float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
        float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));

        vec3 H  = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NoL = max(L.z, 0.0);
        float NoH = max(H.z, 0.0);
        float VoH = max(dot(V, H), 0.0);

        if (NoL > 0.0)
        {
            float G  = G_Smith(NoV, NoL, a);
            float Gv = (G * VoH) / max(NoH * NoV, 1e-5);
            float Fc = pow(1.0 - VoH, 5.0);

            A += (1.0 - Fc) * Gv;
            B += Fc * Gv;
        }
    }

    A /= float(SAMPLES);
    B /= float(SAMPLES);
    return vec2(A, B);
}

void main()
{
    float NoV   = clamp(v_UV.x, 0.0, 1.0);
    float rough = clamp(v_UV.y, 0.0, 1.0);
    FragColor   = IntegrateBRDF(NoV, rough);
}

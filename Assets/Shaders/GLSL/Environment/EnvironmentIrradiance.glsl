#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos;

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_Dir;

void main()
{
    v_Dir       = a_Pos;
    gl_Position = (u_Proj*u_View*vec4(a_Pos,1.0)).xyww;
}

#type fragment
#version 460 core

in vec3 v_Dir;
layout(location=0) out vec4 FragColor;

uniform samplerCube u_EnvironmentTexture; // original env cube

const uint  SAMPLES = 2048u;  // can be tuned at compile time
const float PI      = 3.14159265359;

// Cosine-weighted hemisphere sampling ---------------------------------------------------------
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint n)
{
    return vec2(float(i)/float(n), RadicalInverse_VdC(i));
}

vec3 CosineSample(vec2 Xi)
{
    float r   = sqrt(Xi.x);
    float phi = 2.0 * PI * Xi.y;
    float x   = r * cos(phi);
    float y   = r * sin(phi);
    float z   = sqrt(max(0.0, 1.0 - x*x - y*y));
    return vec3(x,y,z);
}

void main()
{
    vec3 N = normalize(v_Dir);

    // Build TBN with N as Z
    vec3 up    = (abs(N.z) < 0.999) ? vec3(0,0,1) : vec3(1,0,0);
    vec3 T     = normalize(cross(up, N));
    vec3 B     = cross(N, T);
    mat3 TBN   = mat3(T, B, N);

    vec3 acc   = vec3(0);
    float wsum = 0.0;

    for(uint i = 0u; i < SAMPLES; ++i)
    {
        vec2 Xi  = Hammersley(i, SAMPLES);
        vec3 Ls  = CosineSample(Xi);
        vec3 L   = normalize(TBN * Ls);
        float NoL= max(dot(N, L), 0.0);

        acc  += texture(u_EnvironmentTexture, L).rgb * NoL;
        wsum += NoL;
    }

    vec3 irradiance = acc / max(wsum, 1e-4);
    FragColor       = vec4(irradiance, 1.0);
}
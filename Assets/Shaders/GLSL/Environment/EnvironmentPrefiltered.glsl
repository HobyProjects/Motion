#type vertex
#version 460 core

layout(location = 0) in vec3 a_Pos;

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

layout(location=0) out vec4 FragColor;

in vec3 v_Dir; 
uniform samplerCube u_EnvironmentTexture;       // original env cube (mipmapped)
uniform float       u_PrefilteredRoughness;     // [0,1]

const float PI = 3.14159265359;

float radicalInverse_VdC(uint bits)
{
    bits = ( bits << 16u) | ( bits >> 16u);
    bits = (( bits & 0x55555555u ) << 1u ) | (( bits & 0xAAAAAAAAu ) >> 1u);
    bits = (( bits & 0x33333333u ) << 2u ) | (( bits & 0xCCCCCCCCu ) >> 2u);
    bits = (( bits & 0x0F0F0F0Fu ) << 4u ) | (( bits & 0xF0F0F0F0u ) >> 4u);
    bits = (( bits & 0x00FF00FFu ) << 8u ) | (( bits & 0xFF00FF00u ) >> 8u);
    
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{ 
    return vec2(float(i) / float(N), radicalInverse_VdC(i)); 
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float alpha)
{
    float a2        = alpha * alpha; 
    float phi       = 2.0 * PI * Xi.x;

    float cosTheta  = sqrt((1.0 - Xi.y)/(1.0 + (a2 - 1.0) * Xi.y));
    float sinTheta  = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));

    vec3 H  = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
    vec3 up = abs(N.z) < 0.999? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 T  = normalize(cross(up, N)); 
    vec3 B  = cross(N,T);

    return normalize(T * H.x + B * H.y + N * H.z);
}

float D_GGX(float NoH, float a)
{ 
    float a2    = a * a; 
    float d     = (NoH * NoH) * (a2 - 1.0) + 1.0;

    return a2 / (PI * d * d); 
}

void main()
{
    vec3 N              = normalize(v_Dir); 
    vec3 V              = N; 
    float rough         = clamp(u_PrefilteredRoughness, 0.0, 1.0); 
    float a             = max(rough * rough, 1e-4);
    
    const uint SAMPLES  = 1024u; 
    vec3       sum      = vec3(0); 
    float      wsum     = 0.0;
    float      envRes   = float(textureSize(u_EnvironmentTexture,0).x);
    float      maxMip   = float(textureQueryLevels(u_EnvironmentTexture) - 1);

    for(uint i = 0u; i < SAMPLES; ++i)
    {
        vec2 Xi     = Hammersley(i,SAMPLES);
        vec3 H      = ImportanceSampleGGX(Xi,N,a); float VoH=max(dot(V,H),0.0);
        vec3 L      = normalize(2.0*VoH*H - V);
        float NoL   = max(dot(N,L),0.0); float NoH=max(dot(N,H),0.0);

        if(NoL > 0.0)
        {
            float pdf       = max(D_GGX(NoH, a) * NoH / max(4.0 * VoH, 1e-4), 1e-6);
            float saTexel   = 4.0 * PI/(6.0 * envRes * envRes);
            float saSample  = 1.0 / (float(SAMPLES) * pdf);
            float mip       = 0.5 * log2(saSample / saTexel);
            vec3 Li         = textureLod(u_EnvironmentTexture, L, clamp(mip,0.0,maxMip)).rgb;

            sum     += Li*NoL; 
            wsum    += NoL;
        }
    }
    
    vec3 prefiltered    =  sum / max(wsum, 1e-4);
    FragColor           =  vec4(prefiltered, 1.0);
}
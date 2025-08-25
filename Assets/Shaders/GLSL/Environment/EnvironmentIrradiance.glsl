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

layout(location=0) out vec4 FragColor;

in vec3 v_Dir; 

uniform samplerCube u_EnvironmentTexture; // source env cube (mipmapped)

const float PI = 3.14159265359;

// Hammersley + cosine hemisphere sampling (stable at 32–64px outputs)
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | ( bits >> 16u);
    bits = ((bits & 0x55555555u) <<1u) | (( bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) <<2u) | (( bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) <<4u) | (( bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) <<8u) | (( bits & 0xFF00FF00u) >> 8u);

    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N)
{ 
    return vec2(float(i)/float(N), radicalInverse_VdC(i)); 
}

mat3 MakeTBN(vec3 n)
{ 
    vec3 up = abs(n.z) <0.999 ? vec3(0 , 0, 1) : vec3(0,1,0); 
    vec3 t  = normalize(cross(up,n)); 
    vec3 b  = cross(n,t); 

    return mat3(t, b, n);
} 

vec3 CosineSample(vec2 Xi)
{ 
    float phi       = 2.0 * PI * Xi.x; 
    float cosTheta  = sqrt(1.0-Xi.y); 
    float sinTheta  = sqrt(max(0.0,1.0-cosTheta*cosTheta)); 

    return vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
} 

void main()
{
    vec3       N        = normalize(v_Dir); 
    mat3       TBN      = MakeTBN(N);
    const uint SAMPLES  = 1024u; 
    vec3       acc      = vec3(0); 
    float      wsum     = 0.0;

    for(uint i = 0u; i < SAMPLES; ++i)
    { 
        vec2 Xi     = Hammersley(i,SAMPLES); 
        vec3 Ls     = CosineSample(Xi); 
        vec3 L      = normalize(TBN * Ls); 
        float NoL   = max(dot(N, L),0.0); 

        acc  += texture(u_EnvironmentTexture,L).rgb * NoL; 
        wsum += NoL; 
    }

    vec3 irradiance = acc / max(wsum, 1e-4);
    FragColor       = vec4(irradiance, 1.0);
}

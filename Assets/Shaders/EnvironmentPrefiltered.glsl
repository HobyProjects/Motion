#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; 

uniform mat4 u_View; 
uniform mat4 u_Proj; 

out vec3 v_Dir; 

void main()
{ 
    v_Dir=a_Pos; 
    gl_Position=u_Proj*u_View*vec4(a_Pos,1.0); 
}

#type fragment
#version 460 core

in vec3 v_Dir; 
out vec4 FragColor; 

uniform samplerCube u_EnvironmentTexture; 
uniform float u_PrefilteredRoughness; // 0..1

const float PI=3.14159265359;

float VanDerCorput(uint n, uint base)
{ 
    float invBase=1.0/float(base), denom=1.0, res=0.0; 
    for(;n>0u;n/=base)
    { 
        denom*=base; 
        res += float(n%base)/denom; 
    } 
    return res; 
}

vec2 Hammersley(uint i, uint N)
{ 
    return vec2(float(i)/float(N), VanDerCorput(i,2u)); 
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{ 
    float a=roughness*roughness; 
    float phi=2.0*PI*Xi.x; 
    float cosTheta = sqrt((1.0 - Xi.y)/(1.0 + (a*a - 1.0) * Xi.y)); 
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta); 
    vec3 H = vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta); 
    vec3 up = abs(N.z)<0.999? vec3(0,0,1): vec3(1,0,0); 
    vec3 T = normalize(cross(up, N)); 
    vec3 B = cross(N,T); 
    return normalize(T*H.x + B*H.y + N*H.z); 
}

float GeometrySchlickGGX(float NdotV, float rough)
{ 
    float r=rough+1.0; 
    float k=(r*r)/8.0; 
    return NdotV/(NdotV*(1.0-k)+k); 
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{ 
    float NdotV=max(dot(N,V),0.0); 
    float NdotL=max(dot(N,L),0.0); 
    float ggx2=GeometrySchlickGGX(NdotV,rough); 
    float ggx1=GeometrySchlickGGX(NdotL,rough); 
    return ggx1*ggx2; 
}

void main()
{
    vec3 N = normalize(v_Dir);
    vec3 R = N;
    vec3 V = R;
    const uint SAMPLE_COUNT = 1024u;
    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;
    for(uint i=0u;i<SAMPLE_COUNT;++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, u_PrefilteredRoughness);
        vec3 L  = normalize(2.0*dot(V,H)*H - V);
        float NdotL = max(dot(N,L), 0.0);
        if(NdotL>0.0)
        {
            prefiltered += texture(u_EnvironmentTexture, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefiltered = prefiltered / max(totalWeight, 1e-4);
    FragColor = vec4(prefiltered, 1.0);
}

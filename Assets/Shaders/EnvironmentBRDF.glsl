#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; 
layout(location=1) in vec2 a_UV; 

out vec2 v_UV; 

void main()
{ 
    v_UV=a_UV; 
    gl_Position=vec4(a_Pos,1.0); 
}

#type fragment
#version 460 core

in vec2 v_UV; 

out vec2 FragColor; // RG16F target

const float PI=3.14159265359;

float GeometrySchlickGGX(float NdotV, float rough)
{
    float r=rough+1.0;
    float k=(r*r)/8.0;
    return NdotV/(NdotV*(1.0-k)+k);
}

float GeometrySmith(float NdotV, float NdotL, float rough)
{
    float ggx2=GeometrySchlickGGX(NdotV,rough);
    float ggx1=GeometrySchlickGGX(NdotL,rough);
    return ggx1*ggx2;
}

vec2 IntegrateBRDF(float NdotV, float rough)
{
    vec3 V = vec3(sqrt(1.0-NdotV*NdotV),0.0,NdotV);
    float A=0.0, B=0.0;
    const uint SAMPLE_COUNT=1024u;
    for(uint i=0u;i<SAMPLE_COUNT;++i)
    {
        float E1 = float(i)/float(SAMPLE_COUNT);
        float E2 = fract(sin(float(i)*43758.5453123));
        float a = rough*rough;
        float phi = 2.0*PI*E1;
        float cosTheta = sqrt((1.0-E2)/(1.0+(a*a-1.0)*E2));
        float sinTheta = sqrt(max(1.0 - cosTheta*cosTheta, 0.0));
        vec3 H = vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
        vec3 L = normalize(2.0*dot(V,H)*H - V);
        float NdotL = max(L.z,0.0);
        float NdotH = max(H.z,0.0);
        float VdotH = max(dot(V,H),0.0);
        if(NdotL>0.0)
        {
            float G = GeometrySmith(NdotV, NdotL, rough);
            float G_Vis = (G * VdotH) / max(NdotH * NdotV, 1e-4);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A/=float(SAMPLE_COUNT);
    B/=float(SAMPLE_COUNT);
    return vec2(A,B);
}

void main()
{
    float NdotV = v_UV.x;
    float rough = v_UV.y;
    FragColor = IntegrateBRDF(NdotV, rough);
}

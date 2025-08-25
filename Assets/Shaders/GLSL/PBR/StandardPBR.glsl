#type vertex
#version 460 core

layout (location = 0) vec3 aPosition;
layout (location = 1) vec2 aTexCoord;
layout (location = 2) vec3 aNormal;
layout (location = 3) vec4 aTangent;
layout (location = 4) vec3 aBitangent;

layout(std140, binding = 0) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
    float _pad0;
};

layout(std140, binding = 1) uniform Object
{
    mat4 uModel;
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
    T           = normalize(cross(N, T));
    vec3 B      = normalize(cross(N, T)) * aTangent.w;

    vTangentWS      = T;
    vBitangentWS    = B;

    gl_Position     = uProj * uView * wp;
}

#type fragment
#version 460 core



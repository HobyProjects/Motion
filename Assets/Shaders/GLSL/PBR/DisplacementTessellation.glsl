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

#type tessellation_control
#version 460 core

layout (vertices = 3) out;

layout(std140, binding = 0) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; 
    float _pad0;
};

// VS outputs in:
layout(location = 0) in vec3 vWorldPos_VS[];
layout(location = 1) in vec2 vUV_VS[];
layout(location = 2) in vec3 vNormalWS_VS[];
layout(location = 3) in vec3 vTangentWS_VS[];
layout(location = 4) in vec3 vBitangentWS_VS[];

// TCS outputs forward to TES:
layout(location = 0) out vec3 vWorldPos_TCS[];
layout(location = 1) out vec2 vUV_TCS[];
layout(location = 2) out vec3 vNormalWS_TCS[];
layout(location = 3) out vec3 vTangentWS_TCS[];
layout(location = 4) out vec3 vBitangentWS_TCS[];

// Tessellation controls
uniform float uTessMin      = 2.0;
uniform float uTessMax      = 8.0;
uniform float uPixPerEdge   = 20.0;  // target pixels per edge
uniform float uLODNear      = 5.0;   // meters
uniform float uLODFar       = 50.0;  // meters

// Project to NDC for a screen-space length estimate
float edgeScreenPixels(vec3 A, vec3 B) 
{
    vec4 a = uProj * uView * vec4(A, 1.0);
    vec4 b = uProj * uView * vec4(B, 1.0);
    a.xyz /= a.w; b.xyz /= b.w;

    // convert NDC delta to pixels (approx, assume 1080p; make a uniform if needed)
    float pix = length(a.xy - b.xy) * 0.5 * 1080.0;
    return pix;
}

float edgeTess(vec3 A, vec3 B) 
{
    float pix = edgeScreenPixels(A, B);
    float base = clamp(pix / max(uPixPerEdge, 1.0), 1.0, 64.0);

    // Distance-based attenuation (same for both verts → symmetric)
    float d = 0.5 * (distance(uCameraPos, A) + distance(uCameraPos, B));
    float lod = 1.0 - smoothstep(uLODNear, uLODFar, d);

    float t = base * (0.5 + 0.5 * lod);
    // Round to reduce crack risk across shared edges
    t = floor(t + 0.5);
    return clamp(t, uTessMin, uTessMax);
}

void main() 
{
    // Pass through per-vertex data
    vWorldPos_TCS[gl_InvocationID]      = vWorldPos_VS[gl_InvocationID];
    vUV_TCS[gl_InvocationID]            = vUV_VS[gl_InvocationID];
    vNormalWS_TCS[gl_InvocationID]      = vNormalWS_VS[gl_InvocationID];
    vTangentWS_TCS[gl_InvocationID]     = vTangentWS_VS[gl_InvocationID];
    vBitangentWS_TCS[gl_InvocationID]   = vBitangentWS_VS[gl_InvocationID];

    // Compute tess levels once per patch
    if (gl_InvocationID == 0) 
    {
        vec3 A      = vWorldPos_VS[0], B = vWorldPos_VS[1], C = vWorldPos_VS[2];
        float e0    = edgeTess(B, C);
        float e1    = edgeTess(C, A);
        float e2    = edgeTess(A, B);

        gl_TessLevelOuter[0] = e0;
        gl_TessLevelOuter[1] = e1;
        gl_TessLevelOuter[2] = e2;
        gl_TessLevelInner[0] = (e0 + e1 + e2) / 3.0;
    }
}

#type tessellation_evaluation
#version 460 core

layout(triangles, equal_spacing, cw) in;

layout(std140, binding = 0) uniform Camera 
{
    mat4 uView;
    mat4 uProj;
    vec3 uCameraPos; float _pad0;
};

layout(binding = 0) uniform sampler2D uDisplacementTexture; // R channel used

// World scale/bias for displacement
uniform float uDisplacementScale = 0.05; // world units (meters)
uniform float uDisplacementBias  = 0.0;

// Inputs from TCS
layout(location = 0) in vec3 vWorldPos_TCS[];
layout(location = 1) in vec2 vUV_TCS[];
layout(location = 2) in vec3 vNormalWS_TCS[];
layout(location = 3) in vec3 vTangentWS_TCS[];
layout(location = 4) in vec3 vBitangentWS_TCS[];

// Outputs to FS (match standard VS outputs)
layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vNormalWS;     // geometric normal (pre-normalmap)
layout(location = 3) out vec3 vTangentWS;
layout(location = 4) out vec3 vBitangentWS;

void main() 
{
    vec3 b = vec3(gl_TessCoord.x, gl_TessCoord.y, gl_TessCoord.z);

    // Interpolate attributes
    vec3 P  = vWorldPos_TCS[0] * b.x + vWorldPos_TCS[1] * b.y + vWorldPos_TCS[2] * b.z;
    vec2 UV = vUV_TCS[0] * b.x + vUV_TCS[1] * b.y + vUV_TCS[2] * b.z;

    // Reconstruct geometric normal from the interp’d basis for stability
    vec3 N0 = normalize(vNormalWS_TCS[0]);
    vec3 N1 = normalize(vNormalWS_TCS[1]);
    vec3 N2 = normalize(vNormalWS_TCS[2]);
    vec3 N  = normalize(N0 * b.x + N1 * b.y + N2 * b.z);

    // Sample height and displace along the geometric normal
    float h     = texture(uDisplacementTexture, UV).r; // [0,1]
    float disp  = h * uDisplacementScale + uDisplacementBias;
    vec3 Pdisp  = P + N * disp;

    // Interpolate T/B and re-orthogonalize to displaced normal
    vec3 T  = normalize(vTangentWS_TCS[0]*b.x + vTangentWS_TCS[1]*b.y + vTangentWS_TCS[2]*b.z);
    T       = normalize(T - N * dot(N, T));
    vec3 B  = normalize(cross(N, T)); // handedness already applied in VS

    // Outputs
    vWorldPos    = Pdisp;
    vUV          = UV;
    vNormalWS    = N;
    vTangentWS   = T;
    vBitangentWS = B;

    gl_Position = uProj * uView * vec4(Pdisp, 1.0);
}
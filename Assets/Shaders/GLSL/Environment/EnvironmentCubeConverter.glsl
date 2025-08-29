#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; // unit cube positions

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_LocalDir;

void main()
{
    v_LocalDir  = a_Pos;
    gl_Position = u_Proj * u_View * vec4(a_Pos,1.0);
}

#type fragment
#version 460 core

in vec3 v_LocalDir;
layout(location=0) out vec4 FragColor;

uniform sampler2D u_EquiRectangular;

const float PI = 3.14159265359;

vec2 SampleSphericalMap(vec3 v)
{
    vec3  d   = normalize(v);
    float phi = atan(d.z, d.x);
    float the = acos(clamp(d.y, -1.0, 1.0));
    return vec2(phi / (2.0 * PI) + 0.5, the / PI);
}

void main()
{
    vec2 uv   = SampleSphericalMap(v_LocalDir);
    vec3 hdr  = texture(u_EquiRectangular, uv).rgb;
    FragColor = vec4(hdr, 1.0);
}
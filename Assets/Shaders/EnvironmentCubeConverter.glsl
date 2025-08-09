#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; // cube

uniform mat4 u_View; 
uniform mat4 u_Proj;

out vec3 v_LocalDir;

void main()
{ 
    v_LocalDir=a_Pos; 
    gl_Position = u_Proj * u_View * vec4(a_Pos,1.0); 
}

#type fragment
#version 460 core

in vec3 v_LocalDir; out vec4 FragColor;

uniform sampler2D u_EquiRectangular; // UniformCache::EquiRectangular

const float PI=3.14159265359;

vec2 SampleSphericalMap(vec3 v)
{ 
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y)); 
    uv *= vec2(0.15915494309, 0.31830988618); 
    uv += 0.5; 
    return uv; 
}
void main()
{ 
    vec3 dir = normalize(v_LocalDir); 
    vec2 uv = SampleSphericalMap(dir); 
    vec3 c = texture(u_EquiRectangular, uv).rgb; 
    FragColor=vec4(c,1.0); 
}
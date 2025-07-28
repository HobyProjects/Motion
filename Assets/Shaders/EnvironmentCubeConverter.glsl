#type vertex
#version 460 core
layout (location = 0) in vec3 a_Position;

out vec3 v_WorldPosition;

uniform mat4 u_ProjectionMatrix;
uniform mat4 u_ViewMatrix;

void main()
{
    v_WorldPosition = a_Position;  
    gl_Position =  u_ProjectionMatrix * u_ViewMatrix * vec4(v_WorldPosition, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;
in vec3 v_WorldPosition;

uniform sampler2D u_EquiRectangular;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(v_WorldPosition));
    vec3 color = texture(u_EquiRectangular, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
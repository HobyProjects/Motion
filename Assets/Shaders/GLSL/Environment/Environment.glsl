#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos;

uniform mat4 u_View; 
uniform mat4 u_Proj; 

out vec3 v_Dir;

void main()
{ 
    v_Dir       = a_Pos; 
    gl_Position = (u_Proj * u_View * vec4(a_Pos, 1.0)).xyww; 
}

#type fragment
#version 460 core

in vec3 v_Dir; 

layout(location=0) out vec4 FragColor; 

uniform samplerCube u_EnvironmentTexture;

void main()
{ 
    vec3 c      =   texture(u_EnvironmentTexture, normalize(v_Dir)).rgb; 
    FragColor   =   vec4(c, 1.0); 
}
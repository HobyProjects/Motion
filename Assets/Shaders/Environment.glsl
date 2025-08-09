#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; // cube

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_Dir;

void main()
{
    v_Dir = a_Pos; // direction from cube vertex
    vec4 pos = u_Proj * u_View * vec4(a_Pos, 1.0);
    gl_Position = pos.xyww; // push to far plane
}

#type fragment
#version 460 core

in vec3 v_Dir; out vec4 FragColor;

uniform samplerCube u_EnvironmentTexture; // set by UniformCache::EnvironmentTexture
void main()
{
    vec3 dir = normalize(v_Dir);
    vec3 color = texture(u_EnvironmentTexture, dir).rgb;
    FragColor = vec4(color, 1.0);
}
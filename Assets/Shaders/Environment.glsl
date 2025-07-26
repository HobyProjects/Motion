#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;

out vec3 v_TexCoord;

uniform mat4 u_ProjectionMatrix;
uniform mat4 u_ViewMatrix;

void main()
{
    v_TexCoord = a_Position;
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

in vec3 v_TexCoord;

out vec4 FragColor;

uniform samplerCube u_EnvironmentTexture;

void main()
{
    FragColor = texture(u_EnvironmentTexture, v_TexCoord);
}
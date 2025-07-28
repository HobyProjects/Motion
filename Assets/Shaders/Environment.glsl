#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;

out vec3 v_WorldPosition;

uniform mat4 u_ProjectionMatrix;
uniform mat4 u_ViewMatrix;

void main()
{   
    v_WorldPosition = a_Position;
    mat4 rotView = mat4(mat3(u_ViewMatrix));
    gl_Position = u_ProjectionMatrix * rotView * vec4(v_WorldPosition, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;
in vec3 v_WorldPosition;

uniform samplerCube u_EnvironmentTexture;

void main()
{
    vec3 envColor = textureLod(u_EnvironmentTexture, v_WorldPosition, 0.0).rgb;

    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    FragColor = vec4(envColor, 1.0);
}
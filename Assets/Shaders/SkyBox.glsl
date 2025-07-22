#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_TexCoords;

out vec3 v_TexCoords;

uniform mat4 u_ProjectionMatrix;
uniform mat4 u_ViewMatrix;

void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

in vec3 v_TexCoords;

uniform samplerCube u_Skybox;

out vec4 FragColor;

void main() {
    FragColor = texture(u_Skybox, v_TexCoords);
}

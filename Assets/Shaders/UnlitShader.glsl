#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

out vec2 v_TexCoords;

void main() {
    v_TexCoords = a_TexCoords;
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

in vec2 v_TexCoords;

uniform vec3 u_EmissiveColor;
uniform sampler2D u_EmissiveTexture;

out vec4 FragColor;

void main() {
    vec3 emissive = texture(u_EmissiveTexture, v_TexCoords).rgb * u_EmissiveColor;
    FragColor = vec4(emissive, 1.0);
}

#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec2 v_TexCoord;
uniform sampler2D u_PostProcessTexture;

void main() {
    FragColor = texture(u_PostProcessTexture, v_TexCoord);
}
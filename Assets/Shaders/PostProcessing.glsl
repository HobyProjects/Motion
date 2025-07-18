#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Normals;
layout(location = 4) in vec3 a_Tangent;
layout(location = 5) in vec3 a_Bitangent;

out vec2 v_TexCoord;

void main() {
    gl_Position = vec4(a_Position, 1.0);
    v_TexCoord = a_TexCoord;
}

#type fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

in vec2 v_TexCoord;
uniform sampler2D u_PostProcessTexture;

void main() {
    FragColor = texture(u_PostProcessTexture, v_TexCoord);
}
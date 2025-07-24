#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;
layout(location = 3) in vec4 a_Tangent;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjMatrix;

out vec3 v_WorldPosition;
out vec2 v_UV;
out mat3 v_TBN;

void main() {
    vec3 T = normalize(mat3(u_ModelMatrix) * a_Tangent.xyz);
    vec3 N = normalize(mat3(u_ModelMatrix) * a_Normals);
    vec3 B = normalize(cross(N, T)) * a_Tangent.w;

    v_TBN = mat3(T, B, N);
    v_UV = a_TexCoords;
    v_WorldPosition = vec3(u_ModelMatrix * vec4(a_Position, 1.0));

    gl_Position = u_ViewProjMatrix * vec4(v_WorldPosition, 1.0);
}

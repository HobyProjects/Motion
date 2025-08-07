#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;
layout(location = 3) in vec3 a_Tangents;
layout(location = 4) in vec3 a_Bitangents;
layout(location = 5) in float a_TangentSign;

out vec3 v_WorldPosition;
out vec2 v_UV;
out vec3 v_Normal;
out vec3 v_ViewDir;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform mat3 u_NormalMatrix;
uniform vec3 u_CameraPosition;

void main()
{
    v_UV = a_TexCoords;
    v_Normal = normalize(u_NormalMatrix * a_Normals);
    v_WorldPosition = vec3(u_ModelMatrix * vec4(a_Position, 1.0));
    v_ViewDir = normalize(u_CameraPosition - v_WorldPosition);
    gl_Position = u_ProjectionMatrix * u_ViewMatrix * vec4(v_WorldPosition, 1.0);
}

#type fragment
#version 460 core

layout(location = 0) out vec4 FragColor;

in vec3 v_WorldPosition;
in vec2 v_UV;
in vec3 v_Normal;
in vec3 v_ViewDir;

// Texture Samplers
uniform sampler2D u_DiffuseTexture;
uniform sampler2D u_SpecularTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_OpacityTexture;

// IBL
uniform samplerCube u_IrradianceTextures;
uniform samplerCube u_PrefilteredTextures;

// Light & Camera
uniform vec3 u_LightPosition;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

// Material Buffer
struct StandardMaterialAttribute {
    vec3 DiffuseColor;
    vec3 SpecularColor;
    vec3 AmbientColor;
    vec3 EmissiveColor;
    float Shininess;
    float Opacity;
};

layout(std430, binding = 1) buffer MaterialData {
    StandardMaterialAttribute attributes;
};

void main()
{
    // Sample textures if present, else use attributes.
    vec3 diffuseColor = texture(u_DiffuseTexture, v_UV).rgb * attributes.DiffuseColor;
    vec3 specularColor = texture(u_SpecularTexture, v_UV).rgb * attributes.SpecularColor;
    vec3 emissiveColor = texture(u_EmissiveTexture, v_UV).rgb + attributes.EmissiveColor;

    float opacity = texture(u_OpacityTexture, v_UV).r * attributes.Opacity;
    if (opacity < 0.01)
        discard;

    // Ambient (IBL Diffuse)
    vec3 N = normalize(v_Normal);
    vec3 ambientIBL = texture(u_IrradianceTextures, N).rgb * attributes.AmbientColor;

    // Lighting (Direct)
    vec3 L = normalize(u_LightPosition - v_WorldPosition);
    vec3 V = normalize(v_ViewDir);
    vec3 R = reflect(-L, N);

    // Diffuse term (Lambert)
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = diffuseColor * NdotL * u_LightColor * u_LightIntensity;

    // Specular term (Phong)
    float RdotV = max(dot(reflect(-L, N), V), 0.0);
    float spec = pow(RdotV, attributes.Shininess);
    vec3 specular = specularColor * spec * u_LightColor * u_LightIntensity;

    // IBL Specular (Phong-ish): Reflect V over N, sample environment, scale by specular color.
    // For classic Phong, use shininess as mip bias or LOD for blurring.
    const float MAX_LOD = 4.0;
    float lod = (1.0 - 1.0 / (attributes.Shininess + 1.0)) * MAX_LOD;
    vec3 reflection = reflect(-V, N);
    vec3 iblSpecular = textureLod(u_PrefilteredTextures, reflection, lod).rgb * specularColor;

    // Final color
    vec3 color = ambientIBL * diffuseColor + diffuse + specular + iblSpecular + emissiveColor;

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, opacity);
}
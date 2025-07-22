#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normals;

uniform mat4 u_ModelMatrix;
uniform mat4 u_ViewProjMatrix;

out vec3 v_WorldPos;
out vec2 v_TexCoords;
out vec3 v_Normal;

void main() {
    vec4 worldPos = u_ModelMatrix * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_TexCoords = a_TexCoords;
    v_Normal = mat3(transpose(inverse(u_ModelMatrix))) * a_Normals;

    gl_Position = u_ViewProjMatrix * worldPos;
}

#type fragment
#version 460 core

in vec3 v_WorldPos;
in vec2 v_TexCoords;
in vec3 v_Normal;

out vec4 FragColor;

// Camera & lighting
uniform vec3 u_CameraPosition;
uniform vec3 u_LightPosition;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

// Material colors & factors
uniform vec3 u_AmbientColor;
uniform vec3 u_DiffuseColor;
uniform vec3 u_SpecularColor;
uniform vec3 u_EmissiveColor;

uniform float u_Shininess;
uniform float u_ShininessStrength;
uniform float u_Opacity;
uniform float u_Reflectivity;

// Textures
uniform sampler2D u_DiffuseTexture;
uniform sampler2D u_AmbientTexture;
uniform sampler2D u_SpecularTexture;
uniform sampler2D u_ShininessTexture;
uniform sampler2D u_EmissiveTexture;
uniform sampler2D u_OpacityTexture;
uniform sampler2D u_NormalMapsTexture;

// Environment map for reflection
uniform samplerCube u_EnvironmentTexture;

vec3 applyNormalMap() {
    vec3 normalMap = texture(u_NormalMapsTexture, v_TexCoords).rgb;
    normalMap = normalMap * 2.0 - 1.0;
    return normalize(normalMap);
}

void main() {
    vec3 N = applyNormalMap();
    vec3 V = normalize(u_CameraPosition - v_WorldPos);
    vec3 L = normalize(u_LightPosition - v_WorldPos);
    vec3 R = reflect(-V, N);

    float diff = max(dot(N, L), 0.0);
    float shininessTex = texture(u_ShininessTexture, v_TexCoords).r;
    float spec = pow(max(dot(reflect(-L, N), V), 0.0), shininessTex * u_Shininess);

    // Sample textures
    vec3 ambientMap = texture(u_AmbientTexture, v_TexCoords).rgb;
    vec3 diffuseMap = texture(u_DiffuseTexture, v_TexCoords).rgb;
    vec3 specularMap = texture(u_SpecularTexture, v_TexCoords).rgb;
    vec3 emissiveMap = texture(u_EmissiveTexture, v_TexCoords).rgb;
    float opacity = texture(u_OpacityTexture, v_TexCoords).r * u_Opacity;

    // Phong contributions
    vec3 ambient = ambientMap * u_AmbientColor;
    vec3 diffuse = diffuseMap * u_DiffuseColor * diff;
    vec3 specular = specularMap * u_SpecularColor * spec * u_ShininessStrength;
    vec3 emissive = emissiveMap * u_EmissiveColor;

    vec3 lightResult = (ambient + diffuse + specular) * u_LightColor * u_LightIntensity + emissive;

    // Reflectivity (cube map)
    vec3 reflected = texture(u_EnvironmentTexture, R).rgb;
    vec3 finalColor = mix(lightResult, reflected, clamp(u_Reflectivity, 0.0, 1.0));

    FragColor = vec4(finalColor, opacity);
}
#type vertex
#version 460 core
layout(location = 0) in vec3 a_Position;

out vec3 v_WorldPositions;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

void main() {

    mat4 rotationOnlyView = mat4(mat3(u_ViewMatrix));
    v_WorldPositions = a_Position;


    gl_Position = u_ProjectionMatrix * rotationOnlyView * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec3 v_WorldPositions;
uniform samplerCube u_EnvironmentTexture;

const float PI = 3.14159265359;

void main() {

    vec3 N = normalize(v_WorldPositions);
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    vec3 irradiance = vec3(0.0);

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            vec3 tangentSample = vec3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            vec3 sampleVec = tangentSample.x * right +
                             tangentSample.y * up +
                             tangentSample.z * N;

            irradiance += texture(u_EnvironmentTexture, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / nrSamples);
    FragColor = vec4(irradiance, 1.0);
}

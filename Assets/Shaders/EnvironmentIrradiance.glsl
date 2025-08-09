#type vertex
#version 460 core

layout(location=0) in vec3 a_Pos; 

uniform mat4 u_View;
uniform mat4 u_Proj;

out vec3 v_Dir;

void main()
{
    v_Dir=a_Pos;
    gl_Position=u_Proj*u_View*vec4(a_Pos,1.0);
}

#type fragment
#version 460 core

in vec3 v_Dir; out vec4 FragColor; 

uniform samplerCube u_EnvironmentTexture; // source cube

const float PI=3.14159265359;

void main()
{
    // Simple cosine-weighted convolution approximation
    vec3 N = normalize(v_Dir);
    vec3 up = vec3(0.0,1.0,0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    float sampleDelta = 0.025;
    vec3 irradiance = vec3(0.0);
    float nrSamples = 0.0;

    for(float phi=0.0; phi<2.0*PI; phi+=sampleDelta)
    {
        for(float theta=0.0; theta<0.5*PI; theta+=sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x*right + tangentSample.y*up + tangentSample.z*N;
            irradiance += texture(u_EnvironmentTexture, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    irradiance = PI * irradiance * (1.0 / max(nrSamples,1.0));
    FragColor = vec4(irradiance, 1.0);
}
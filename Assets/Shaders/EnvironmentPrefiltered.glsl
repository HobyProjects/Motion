#type vertex
#version 460 core
layout(location = 0) in vec3 a_Position;

out vec3 v_WorldPositions;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;

void main() 
{
    mat4 rotationOnlyView = mat4(mat3(u_ViewMatrix));
    v_WorldPositions = a_Position;
    gl_Position = u_ProjectionMatrix * rotationOnlyView * vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;
in vec3 v_WorldPositions;

uniform samplerCube u_EnvironmentTexture;
uniform float u_PrefilteredRoughness;
uniform float u_PrefilteredResolution;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
/**
 * Calculates the GGX (Trowbridge-Reitz) normal distribution function (NDF) for microfacet-based BRDFs.
 *
 * @param N         The surface normal vector.
 * @param H         The half-vector between view and light directions.
 * @param roughness The surface roughness parameter (typically in [0, 1]).
 * @return          The GGX distribution value for the given inputs.
 *
 * This function implements the GGX distribution, which models the distribution of microfacet normals
 * on a surface, controlling the appearance of specular highlights based on roughness.
 */
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
/**
 * Computes the radical inverse of the given unsigned integer using the Van der Corput sequence in base 2.
 * This function reverses the bits of the input and normalizes the result to the [0, 1) range.
 *
 * @param bits The unsigned integer to compute the radical inverse for.
 * @return The radical inverse value in the range [0, 1).
 *
 * Useful for generating low-discrepancy sequences for sampling in rendering algorithms.
 */
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// ----------------------------------------------------------------------------
/**
 * Generates a 2D Hammersley point for low-discrepancy sampling.
 *
 * @param i The index of the sample point.
 * @param N The total number of sample points.
 * @return A vec2 representing the Hammersley point, where the x component is i/N and the y component is the radical inverse of i.
 *
 * The Hammersley sequence is commonly used in quasi-random sampling for rendering techniques such as importance sampling.
 */
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

// ----------------------------------------------------------------------------
/**
 * ImportanceSampleGGX
 * 
 * Generates a sample vector based on the GGX microfacet distribution for importance sampling in physically-based rendering.
 *
 * @param Xi        A 2D random sample vector (typically in [0, 1] range) used for sampling the hemisphere.
 * @param N         The surface normal vector.
 * @param roughness The roughness parameter of the surface (controls the spread of the distribution).
 * 
 * @return A normalized sample vector in world space, oriented according to the GGX distribution.
 */
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N,  float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

// ----------------------------------------------------------------------------
/**
 * Main fragment shader for environment map prefiltering using importance sampling.
 *
 * This shader computes the prefiltered color for a given surface normal using GGX importance sampling.
 * It is typically used for image-based lighting (IBL) in physically-based rendering (PBR) workflows.
 *
 * Steps:
 * 1. Normalize the input world position to obtain the normal vector (N).
 * 2. Loop over a fixed number of samples (SAMPLE_COUNT) using the Hammersley sequence for low-discrepancy sampling.
 * 3. For each sample:
 *    - Generate a half-vector (H) using GGX importance sampling.
 *    - Compute the light direction (L) from the half-vector and view direction.
 *    - Calculate the weight of the sample based on the GGX distribution and PDF.
 *    - Determine the mipmap level for texture sampling based on roughness and sample area.
 *    - Accumulate the weighted color from the environment map.
 * 4. Normalize the accumulated color by the total weight.
 * 5. Output the final prefiltered color.
 *
 * Uniforms:
 * - u_EnvironmentTexture: Cubemap environment texture.
 * - u_PrefilteredRoughness: Surface roughness parameter for prefiltering.
 * - u_PrefilteredResolution: Resolution of the prefiltered environment map.
 *
 * Inputs:
 * - v_WorldPositions: World-space position or normal vector.
 *
 * Outputs:
 * - FragColor: Prefiltered environment color for the fragment.
 */
void main()
{		
    vec3 N = normalize(v_WorldPositions);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    float resolution = max(u_PrefilteredResolution, 1.0);
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, u_PrefilteredRoughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float D   = DistributionGGX(N, H, u_PrefilteredRoughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = u_PrefilteredRoughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
            prefilteredColor += textureLod(u_EnvironmentTexture, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}
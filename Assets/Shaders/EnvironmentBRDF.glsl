#type vertex
#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;

out vec2 v_TexCoords;

void main() 
{
    v_TexCoords = a_TexCoords;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 460 core

out vec2 FragColor;
in vec2 v_TexCoords;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
/**
 * Computes the radical inverse of the given unsigned integer using the Van der Corput sequence in base 2.
 * This function reverses the bits of the input and normalizes the result to the [0, 1) range.
 *
 * @param bits The unsigned integer to be reversed and normalized.
 * @return The radical inverse value in the range [0, 1).
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
 * Generates a 2D Hammersley point for quasi-random sampling.
 *
 * @param i The index of the sample point.
 * @param N The total number of sample points.
 * @return A vec2 representing the Hammersley point, where the x component is i/N and the y component is the radical inverse of i.
 *
 * The Hammersley sequence is commonly used in importance sampling for rendering,
 * providing low-discrepancy sample distributions.
 */
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}


// ----------------------------------------------------------------------------
/**
 * ImportanceSampleGGX
 * 
 * Generates a sample vector according to the GGX (Trowbridge-Reitz) microfacet distribution,
 * used for importance sampling in physically-based rendering.
 * 
 * @param Xi        A 2D random sample vector (typically in [0, 1] range) used for sampling.
 * @param N         The surface normal vector in world space.
 * @param roughness The material roughness parameter (controls the spread of the distribution).
 * 
 * @return A normalized sample vector in world space, distributed according to GGX.
 */
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}


// ----------------------------------------------------------------------------
/**
 * Calculates the geometry attenuation term using the Schlick-GGX approximation.
 * This function is commonly used in physically based rendering (PBR) for image-based lighting (IBL).
 *
 * @param NdotV     Cosine of the angle between the surface normal and the view direction.
 * @param roughness Surface roughness parameter (typically in [0, 1]).
 * @return          Geometry attenuation factor for the given view direction and roughness.
 */
float GeometrySchlickGGX(float NdotV, float roughness)
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}


// ----------------------------------------------------------------------------
/**
 * Calculates the combined geometric attenuation term using the Smith method for microfacet BRDFs.
 *
 * This function computes the geometry term for both the view direction (V) and the light direction (L)
 * using the Schlick-GGX approximation, and returns their product. This term accounts for shadowing and
 * masking effects due to surface microgeometry.
 *
 * @param N         Surface normal vector.
 * @param V         View direction vector.
 * @param L         Light direction vector.
 * @param roughness Surface roughness parameter (typically in [0, 1]).
 * @return          Combined geometric attenuation factor.
 */
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


// ----------------------------------------------------------------------------
/**
 * Integrates the BRDF for a given view angle and surface roughness using importance sampling.
 *
 * @param NdotV     Cosine of the angle between the surface normal and the view direction.
 * @param roughness Surface roughness parameter (0 = smooth, 1 = rough).
 * @return          A vec2 containing the integrated BRDF values (A, B) for use in image-based lighting.
 *
 * The function uses importance sampling of the GGX microfacet distribution to estimate the split-sum
 * approximation for environment BRDF. It iterates over a fixed number of samples, generating half-vectors
 * and corresponding light directions, and accumulates the visibility and Fresnel terms. The result is
 * averaged over all samples.
 */
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}


// ----------------------------------------------------------------------------
/**
 * Main fragment shader entry point.
 * Computes the integrated BRDF value for the given texture coordinates.
 * 
 * - Uses the IntegrateBRDF function to calculate the BRDF integration based on v_TexCoords.
 * - Assigns the result to FragColor for output.
 */
void main() 
{
    vec2 integratedBRDF = IntegrateBRDF(v_TexCoords.x, v_TexCoords.y);
    FragColor = integratedBRDF;
}
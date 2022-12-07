#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform int SamplesCount;
uniform samplerCube prefilterMap;
uniform samplerCube irradianceMap;

uniform float roughness;
uniform float metallic;
uniform vec3 materialColor;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
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
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
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

float chiGGX(float v)
{
    return v > 0 ? 1 : 0;
}

float GGX_Distribution(vec3 n, vec3 h, float alpha)
{
    float NoH = dot(n,h);
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float den = NoH2 * alpha2 + (1 - NoH2);
    return (chiGGX(NoH) * alpha2) / ( PI * den * den );
}

float GGX_PartialGeometryTerm(vec3 v, vec3 n, vec3 h, float alpha)
{
    float VoH2 = clamp(dot(v,h), 0.0, 1.0);
    float chi = chiGGX( VoH2 / clamp(dot(v,n), 0.0, 1.0) );
    VoH2 = VoH2 * VoH2;
    float tan2 = ( 1 - VoH2 ) / VoH2;
    return (chi * 2) / ( 1 + sqrt( 1 + alpha * alpha * tan2 ) );
}

vec3 Fresnel_Schlick(float cosT, vec3 F0)
{
  return F0 + (1-F0) * pow( 1 - cosT, 5);
}

vec3 GGX_Specular( samplerCube SpecularEnvmap, vec3 normal, vec3 viewVector, float roughness, vec3 F0, out vec3 kS )
{
    vec3 reflectionVector = reflect(-viewVector, normal);
    vec3 radiance = vec3(0, 0, 0);
    float NoV = clamp(dot(normal, viewVector), 0.0, 1.0);
    
    
    for(int i = 0; i < SamplesCount; ++i)
    {
        // Generate a sample vector in some local space
        vec2 Xi = Hammersley(uint(i), uint(SamplesCount));
        vec3 sampleVector = ImportanceSampleGGX(Xi, normal, roughness);

        // Calculate the half vector
        vec3 halfVector = normalize(sampleVector + viewVector);
        float cosT = clamp(dot(sampleVector, normal), 0.0, 1.0);
        float sinT = sqrt( 1 - cosT * cosT);

        // Calculate fresnel
        vec3 fresnel = Fresnel_Schlick( clamp(dot( halfVector, viewVector ), 0.0, 1.0), F0 );
        // Geometry term
        float geometry = GGX_PartialGeometryTerm(viewVector, normal, halfVector, roughness) * GGX_PartialGeometryTerm(sampleVector, normal, halfVector, roughness);
        // Calculate the Cook-Torrance denominator
        float denominator = clamp( 4 * (NoV * clamp(dot(halfVector, normal), 0.0, 1.0) + 0.05), 0.0, 1.0 );
        kS += fresnel;
        // Accumulate the radiance
        radiance += textureLod(prefilterMap, sampleVector, roughness * MAX_REFLECTION_LOD).rgb * geometry * fresnel / denominator;
    }

    // Scale back for the samples count
    kS = clamp( kS / SamplesCount, 0.0, 1.0 );
    return radiance / SamplesCount;        
}

void main()
{  
    

    vec3 F0 = materialColor;
    vec3 I = normalize(Position - cameraPos);
    vec3 R = reflect(I, normalize(Normal));

    F0 = mix(F0, materialColor, metallic);

    vec3 ks = vec3(0, 0, 0);
    vec3 specular = GGX_Specular(prefilterMap, Normal, I, roughness, F0, ks);
    vec3 kd = (1 - ks) * (1 - metallic);

    vec3 irradiance = texture(irradianceMap, Normal).rgb;
    vec3 diffuse = irradiance * materialColor;
    
    float gamma = 2.2;

    FragColor = vec4(specular, 1.0);

    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
}
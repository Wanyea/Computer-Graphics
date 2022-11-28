#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void renderCube();
void renderSphere();


const char* srcVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Position = vec3(model * vec4(position, 1.0));
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)HERE";

const char* srcFS = R"HERE(
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

const float pi = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;


float radicalInverse(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), radicalInverse(i));
}

vec3 importanceSample(vec2 X, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * pi * X.x;
	float cosTheta = sqrt((1.0 - X.y) / (1.0 + (a*a - 1.0) * X.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// from tangent-space H vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float chi(float v)
{
    return v > 0 ? 1 : 0;
}

float distribution(vec3 n, vec3 h, float alpha)
{
    float NoH = dot(n,h);
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float den = NoH2 * alpha2 + (1 - NoH2);
    return (chi(NoH) * alpha2) / ( pi * den * den );
}

float partialGeometry(vec3 v, vec3 n, vec3 h, float alpha)
{
    float VoH2 = clamp(dot(v,h), 0.0, 1.0);
    float chi = chi( VoH2 / clamp(dot(v,n), 0.0, 1.0) );
    VoH2 = VoH2 * VoH2;
    float tan2 = ( 1 - VoH2 ) / VoH2;
    return (chi * 2) / ( 1 + sqrt( 1 + alpha * alpha * tan2 ) );
}

vec3 Fresnel_Schlick(float cosT, vec3 F0)
{
  return F0 + (1-F0) * pow( 1 - cosT, 5);
}

vec3 specular( samplerCube SpecularEnvmap, vec3 normal, vec3 viewVector, float roughness, vec3 F0, out vec3 kS )
{
    vec3 reflectionVector = reflect(-viewVector, normal);
    vec3 radiance = vec3(0, 0, 0);
    float NoV = clamp(dot(normal, viewVector), 0.0, 1.0);
    
    
    for(int i = 0; i < SamplesCount; ++i)
    {
        // Generate a sample vector in some local space
        vec2 X = Hammersley(uint(i), uint(SamplesCount));
        vec3 sampleVector = importanceSample(X, normal, roughness);

        // Calculate the half vector
        vec3 halfVector = normalize(sampleVector + viewVector);
        float cosT = clamp(dot(sampleVector, normal), 0.0, 1.0);
        float sinT = sqrt( 1 - cosT * cosT);

        // Calculate fresnel
        vec3 fresnel = Fresnel_Schlick( clamp(dot( halfVector, viewVector ), 0.0, 1.0), F0 );
        // Geometry term
        float geometry = partialGeometry(viewVector, normal, halfVector, roughness) * partialGeometry(sampleVector, normal, halfVector, roughness);
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
    vec3 partialGeometry = specular(prefilterMap, Normal, I, roughness, F0, ks);
    vec3 kd = (1 - ks) * (1 - metallic);

    vec3 irradiance = texture(irradianceMap, Normal).rgb;
    vec3 diffuse = irradiance * materialColor;
    
    FragColor = vec4(partialGeometry, 1.0);
}
)HERE";

const char* skyboxSourceVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = position;
    vec4 pos = projection * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}  
)HERE";

const char* skyboxSourceFS = R"HERE(
#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}
)HERE";

const char* convertWorldVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;

out vec3 worldPosition;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    worldPosition = position;  
    gl_Position =  projection * view * vec4(worldPosition, 1.0);
}
)HERE";

const char* convertWorldFS = R"HERE(
#version 330 core
out vec4 FragColor;
in vec3 worldPosition;

uniform sampler2D recMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sphereMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = sphereMap(normalize(worldPosition));

    vec3 color = texture(recMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
)HERE";

const char* irradianceSourceVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;

out vec3 worldPosition;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    worldPosition = position;  
    gl_Position =  projection * view * vec4(worldPosition, 1.0);
}
)HERE";

const char* prefilterSourceVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;

out vec3 worldPosition;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    worldPosition = position;  
    gl_Position =  projection * view * vec4(worldPosition, 1.0);
}
)HERE";

const char* irradianceSourceFS = R"HERE(
#version 330 core
out vec4 FragColor;
in vec3 worldPosition;

uniform samplerCube environmentMap;

const float pi = 3.1415926535;

void main()
{		
    vec3 N = normalize(worldPosition);

    vec3 irradiance = vec3(0.0);   
    
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float samples = 0.0f;
    for(float phi = 0.0; phi < 2.0 * pi; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * pi; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));

            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            samples++;
        }
    }

    irradiance = pi * irradiance * (1.0 / float(samples));
    
    FragColor = vec4(irradiance, 1.0);
}
)HERE";

const char* prefilterSourceFS = R"HERE(
#version 330 core
out vec4 FragColor;
in vec3 worldPosition;

uniform samplerCube environmentMap;
uniform float roughness;

const float pi = 3.1415926535;

float distribution(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float aSquared = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH_Squared = NdotH*NdotH;

    float nom   = aSquared;
    float denom = (NdotH_Squared * (aSquared - 1.0) + 1.0);
    denom = pi * denom * denom;

    return nom / denom;
}

float radicalInverse(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), radicalInverse(i));
}

vec3 importanceSample(vec2 X, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * pi * X.x;
	float cosTheta = sqrt((1.0 - X.y) / (1.0 + (a*a - 1.0) * X.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main()
{		
    vec3 N = normalize(worldPosition);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float weight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 X = Hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSample(X, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float D   = distribution(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = 512.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * pi / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            weight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / weight;

    FragColor = vec4(prefilteredColor, 1.0);
}
)HERE";



const float pi = 3.1415926535f;
int whichKeyPressed = 0;
float elevation = pi / 2.0f;
float phi = pi / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int xWindow = 1920;
int yWindow = 1281;


float degreesToRadians(float degree)
{
    return 2 * pi * (degree / 360.0f);
}

static void key_callback(GLFWwindow* window, int key, int keycode, int action, int mods)
{
    float cameraSpeed = static_cast<float>(deltaTime);
    float speed = 18.0f;
    bool ep = false, em = false, ap = false, am = false;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        whichKeyPressed = 0;
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        whichKeyPressed = 1;
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        whichKeyPressed = 2;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS && elevation < degreesToRadians(179)) {
        ep = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
        ep = false;
    }

    if (key == GLFW_KEY_W && action == GLFW_PRESS && elevation > degreesToRadians(1)) {
        em = true;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE) {
        em = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS) {
        ap = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE) {
        ap = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS) {
        am = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE) {
        am = false;
    }

    if (ep)
    {
        elevation += cameraSpeed * speed;
    }

    if (em)
    {
        elevation -= cameraSpeed * speed;
    }

    if (ap)
    {
        phi -= cameraSpeed * speed;
    }

    if (am)
    {
        phi += cameraSpeed * speed;
    }

}

int main(void)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLFWwindow* window;

    if (!glfwInit())
    {
        return -1;
    }


    window = glfwCreateWindow(xWindow, yWindow, "OpenGL Window", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Error: %s\n" <<
            glewGetErrorString(err) << std::endl;
    }

    glfwSetKeyCallback(window, key_callback);


    unsigned int shaderObjVS = glCreateShader(GL_VERTEX_SHADER);
    int vertexLength = (int)strlen(srcVS);
    glShaderSource(shaderObjVS, 1, &srcVS, &vertexLength);
    glCompileShader(shaderObjVS);

    // Check for errors.
    int vertexSuccess;
    glGetShaderiv(shaderObjVS, GL_COMPILE_STATUS, &vertexSuccess);
    if (vertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(shaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(shaderObjVS);
    }

    unsigned int shaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int fragmentLength = (int)strlen(srcFS);
    glShaderSource(shaderObjFS, 1, &srcFS, &fragmentLength);
    glCompileShader(shaderObjFS);

    // Check for errors.
    int fragmentSuccess;
    glGetShaderiv(shaderObjFS, GL_COMPILE_STATUS, &fragmentSuccess);
    if (fragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(shaderObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(shaderObjFS);
    }

    unsigned int ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, shaderObjVS);
    glAttachShader(ShaderProgram, shaderObjFS);
    glLinkProgram(ShaderProgram);

    // Check for errors.
    int shaderSuccess;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &shaderSuccess);
    if (shaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(ShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(ShaderProgram);

    unsigned int skyboxSourceObjVS = glCreateShader(GL_VERTEX_SHADER);
    int skyVertexLength = (int)strlen(skyboxSourceVS);
    glShaderSource(skyboxSourceObjVS, 1, &skyboxSourceVS, &skyVertexLength);
    glCompileShader(skyboxSourceObjVS);

    // Check for errors.
    int skyVertexSuccess;
    glGetShaderiv(skyboxSourceObjVS, GL_COMPILE_STATUS, &skyVertexSuccess);
    if (skyVertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(skyboxSourceObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(skyboxSourceObjVS);
    }

    unsigned int skyboxSourceObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int skyFragmentLength = (int)strlen(skyboxSourceFS);
    glShaderSource(skyboxSourceObjFS, 1, &skyboxSourceFS, &skyFragmentLength);
    glCompileShader(skyboxSourceObjFS);

    // Check for errors.
    int skyFragmentSuccess;
    glGetShaderiv(skyboxSourceObjFS, GL_COMPILE_STATUS, &skyFragmentSuccess);
    if (skyFragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(skyboxSourceObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(skyboxSourceObjFS);
    }

    unsigned int skyShaderProgram = glCreateProgram();
    glAttachShader(skyShaderProgram, skyboxSourceObjVS);
    glAttachShader(skyShaderProgram, skyboxSourceObjFS);
    glLinkProgram(skyShaderProgram);

    // Check for errors.
    int skyShaderSuccess;
    glGetProgramiv(skyShaderProgram, GL_LINK_STATUS, &skyShaderSuccess);
    if (skyShaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(skyShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(skyShaderProgram);

    unsigned int convertWorldObjVS = glCreateShader(GL_VERTEX_SHADER);
    int skyConvertVertexLength = (int)strlen(convertWorldVS);
    glShaderSource(convertWorldObjVS, 1, &convertWorldVS, &skyConvertVertexLength);
    glCompileShader(convertWorldObjVS);

    // Check for errors.
    int skyConvertVertexSuccess;
    glGetShaderiv(convertWorldObjVS, GL_COMPILE_STATUS, &skyConvertVertexSuccess);
    if (skyConvertVertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(convertWorldObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(convertWorldObjVS);
    }

    unsigned int convertWorldObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int skyConvertFragmentLength = (int)strlen(convertWorldFS);
    glShaderSource(convertWorldObjFS, 1, &convertWorldFS, &skyConvertFragmentLength);
    glCompileShader(convertWorldObjFS);

    // Check for errors.
    int skyConvertFragmentSuccess;
    glGetShaderiv(convertWorldObjFS, GL_COMPILE_STATUS, &skyConvertFragmentSuccess);
    if (skyConvertFragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(convertWorldObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(convertWorldObjFS);
    }

    unsigned int convertShaderProgram = glCreateProgram();
    glAttachShader(convertShaderProgram, convertWorldObjVS);
    glAttachShader(convertShaderProgram, convertWorldObjFS);
    glLinkProgram(convertShaderProgram);
    int skyConvertShaderSuccess;
    glGetProgramiv(convertShaderProgram, GL_LINK_STATUS, &skyConvertShaderSuccess);
    if (skyConvertShaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(convertShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(convertShaderProgram);

    unsigned int irradianceSourceObjVS = glCreateShader(GL_VERTEX_SHADER);
    int skyIrradianceVertexLength = (int)strlen(irradianceSourceVS);
    glShaderSource(irradianceSourceObjVS, 1, &irradianceSourceVS, &skyIrradianceVertexLength);
    glCompileShader(irradianceSourceObjVS);

    // Check for errors.
    int skyIrradianceVertexSuccess;
    glGetShaderiv(irradianceSourceObjVS, GL_COMPILE_STATUS, &skyIrradianceVertexSuccess);
    if (skyIrradianceVertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(irradianceSourceObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(irradianceSourceObjVS);
    }

    unsigned int irradianceSourceObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int skyIrradianceFragmentLength = (int)strlen(irradianceSourceFS);
    glShaderSource(irradianceSourceObjFS, 1, &irradianceSourceFS, &skyIrradianceFragmentLength);
    glCompileShader(irradianceSourceObjFS);

    // Check for errors.
    int skyIrradianceFragmentSuccess;
    glGetShaderiv(irradianceSourceObjFS, GL_COMPILE_STATUS, &skyIrradianceFragmentSuccess);
    if (skyIrradianceFragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(irradianceSourceObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(irradianceSourceObjFS);
    }

    unsigned int irradianceShaderProgram = glCreateProgram();
    glAttachShader(irradianceShaderProgram, irradianceSourceObjVS);
    glAttachShader(irradianceShaderProgram, irradianceSourceObjFS);
    glLinkProgram(irradianceShaderProgram);

    // Check for errors.
    int skyIrradianceShaderSuccess;
    glGetProgramiv(irradianceShaderProgram, GL_LINK_STATUS, &skyIrradianceShaderSuccess);
    if (skyIrradianceShaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(irradianceShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(irradianceShaderProgram);

    unsigned int prefilterSourceObjVS = glCreateShader(GL_VERTEX_SHADER);
    int skyPrefilterVertexLength = (int)strlen(prefilterSourceVS);
    glShaderSource(prefilterSourceObjVS, 1, &prefilterSourceVS, &skyPrefilterVertexLength);
    glCompileShader(prefilterSourceObjVS);

    // Check for errors.
    int skyPrefilterVertexSuccess;
    glGetShaderiv(prefilterSourceObjVS, GL_COMPILE_STATUS, &skyPrefilterVertexSuccess);
    if (skyPrefilterVertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(prefilterSourceObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(prefilterSourceObjVS);
    }

    unsigned int prefilterSourceObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int skyPrefilterFragmentLength = (int)strlen(prefilterSourceFS);
    glShaderSource(prefilterSourceObjFS, 1, &prefilterSourceFS, &skyPrefilterFragmentLength);
    glCompileShader(prefilterSourceObjFS);

    // Check for errors.
    int skyPrefilterFragmentSuccess;
    glGetShaderiv(prefilterSourceObjFS, GL_COMPILE_STATUS, &skyPrefilterFragmentSuccess);
    if (skyPrefilterFragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(prefilterSourceObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(prefilterSourceObjFS);
    }

    unsigned int skyPrefilterShaderProgram = glCreateProgram();
    glAttachShader(skyPrefilterShaderProgram, prefilterSourceObjVS);
    glAttachShader(skyPrefilterShaderProgram, prefilterSourceObjFS);
    glLinkProgram(skyPrefilterShaderProgram);

    // Check for errors.
    int skyPrefilterShaderSuccess;
    glGetProgramiv(skyPrefilterShaderProgram, GL_LINK_STATUS, &skyPrefilterShaderSuccess);
    if (skyPrefilterShaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(skyPrefilterShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(skyPrefilterShaderProgram);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_FRAMEBUFFER_SRGB);

    unsigned int capFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &capFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, capFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    stbi_set_flip_vertically_on_load(true);
    int HDRWidth, HDRHeight, HDRComps;
    float* HDR_Data = stbi_loadf("city.hdr", &HDRWidth, &HDRHeight, &HDRComps, 0);
    unsigned int hdrTexture;

    if (HDR_Data != NULL)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, HDRWidth, HDRHeight, 0, GL_RGB, GL_FLOAT, HDR_Data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(HDR_Data);
    } else {
        std::cout << "Failed to load HDR image." << std::endl;
    }

    unsigned int environmentCubmap;
    glGenTextures(1, &environmentCubmap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubmap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, 1024, 1024, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };


    glUseProgram(convertShaderProgram);
    glUniform1i(glGetUniformLocation(convertShaderProgram, "recMap"), 0);
    int convertProjectionLocation = glGetUniformLocation(convertShaderProgram, "projection");
    glUniformMatrix4fv(convertProjectionLocation, 1, GL_FALSE, &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 1920, 1281); 
    glBindFramebuffer(GL_FRAMEBUFFER, capFBO);

    for (unsigned int i = 0; i < 6; ++i)
    {
        int convertViewLocation = glGetUniformLocation(convertShaderProgram, "view");
        glUniformMatrix4fv(convertViewLocation, 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentCubmap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube();

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
 
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubmap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, capFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    glUseProgram(irradianceShaderProgram);
    glUniform1i(glGetUniformLocation(irradianceShaderProgram, "environmentMap"), 0);
    int convertIrradianceProjectionLocation = glGetUniformLocation(irradianceShaderProgram, "projection");
    glUniformMatrix4fv(convertIrradianceProjectionLocation, 1, GL_FALSE, &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubmap);

    glViewport(0, 0, 1920, 1281); 
    glBindFramebuffer(GL_FRAMEBUFFER, capFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        int convertIrradianceViewLocation = glGetUniformLocation(irradianceShaderProgram, "view");
        glUniformMatrix4fv(convertIrradianceViewLocation, 1, GL_FALSE, &captureViews[i][0][0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glUseProgram(skyPrefilterShaderProgram);
    glUniform1i(glGetUniformLocation(skyPrefilterShaderProgram, "environmentMap"), 0);
    int convertPrefilterProjectionLocation = glGetUniformLocation(skyPrefilterShaderProgram, "projection");
    glUniformMatrix4fv(convertPrefilterProjectionLocation, 1, GL_FALSE, &captureProjection[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubmap);

    glBindFramebuffer(GL_FRAMEBUFFER, capFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);

        float roughnessLocation = glGetUniformLocation(skyPrefilterShaderProgram, "roughness");
        glUniform1i(roughnessLocation, roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            int convertPrefilterViewLocation = glGetUniformLocation(skyPrefilterShaderProgram, "view");
            glUniformMatrix4fv(convertPrefilterViewLocation, 1, GL_FALSE, &captureViews[i][0][0]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    float skyboxPositions[] = {

        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned int> sphereIndices;

    std::vector<glm::vec3> secondPositions;
    std::vector<glm::vec3> secondNormals;
    std::vector<glm::vec2> secondUVs;
    std::vector<unsigned int> secondSphereIndices;

    const unsigned int numOfStacks = 256;
    const unsigned int numOfSections = 256;

    float radius = 0.5f;
    for (unsigned int x = 0; x <= numOfStacks; ++x)
    {
        for (unsigned int y = 0; y <= numOfSections; ++y)
        {
            float xSegment = (float)x / (float)numOfStacks;
            float ySegment = (float)y / (float)numOfSections;
            float xPos = (std::cos(xSegment * 2.0f * pi) * std::sin(ySegment * pi)) * radius;
            float yPos = (std::cos(ySegment * pi)) * radius;
            float zPos = (std::sin(xSegment * 2.0f * pi) * std::sin(ySegment * pi)) * radius;

            positions.emplace_back(xPos, yPos, zPos);
            normals.emplace_back(xPos, yPos, zPos);
          
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < numOfSections; ++y)
    {
        if (!oddRow)
        {
            for (unsigned int x = 0; x <= numOfStacks; ++x)
            {
                sphereIndices.push_back(y * (numOfStacks + 1) + x);
                sphereIndices.push_back((y + 1) * (numOfStacks + 1) + x);
            }
        } else {
            for (int x = numOfStacks; x >= 0; --x)
            {
                sphereIndices.push_back((y + 1) * (numOfStacks + 1) + x);
                sphereIndices.push_back(y * (numOfStacks + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    unsigned int indexCount;
    indexCount = static_cast<unsigned int>(sphereIndices.size());

    std::vector<float> data;
    std::vector<glm::vec3> normalLinePoints;
    std::vector<glm::vec3> tangentLinePoints;
    std::vector<glm::vec3> bitangentLinePoints;


    for (unsigned int i = 0, j = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
   
        normalLinePoints.push_back(positions[i]);
        normalLinePoints.emplace_back(1.0f, 0.0f, 0.0f);
        normalLinePoints.push_back(positions[i] + (positions[i] * 0.1f));
        normalLinePoints.emplace_back(1.0f, 0.0f, 0.0f);

        glm::vec3 tangentVar = glm::normalize(glm::cross(positions[i], glm::vec3(0.0f, 0.0f, 1.0f)));

        tangentLinePoints.push_back(positions[i]);
        tangentLinePoints.emplace_back(0.0f, 0.0f, 1.0f);
        tangentLinePoints.push_back(positions[i] + 0.1f * tangentVar);
        tangentLinePoints.emplace_back(0.0f, 0.0f, 1.0f);

        glm::vec3 bitangentVar = glm::normalize(glm::cross(positions[i], tangentVar));

        bitangentLinePoints.push_back(positions[i]);
        bitangentLinePoints.emplace_back(0.0f, 1.0f, 0.0f);
        bitangentLinePoints.push_back(positions[i] + 0.1f * bitangentVar);
        bitangentLinePoints.emplace_back(0.0f, 1.0f, 0.0f);


    }

    
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxPositions), &skyboxPositions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glUseProgram(ShaderProgram);
    glUniform1i(glGetUniformLocation(ShaderProgram, "irradianceMap"), 0);
    glUniform1i(glGetUniformLocation(ShaderProgram, "prefilterMap"), 1);

    glUseProgram(skyShaderProgram);
    glUniform1i(glGetUniformLocation(skyShaderProgram, "skybox"), 0);
    glViewport(0, 0, 1920, 1281);



    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (whichKeyPressed == 0)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else if (whichKeyPressed == 1)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else if (whichKeyPressed == 2)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glPointSize(5.0);
        }

        glUseProgram(ShaderProgram);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f));
        int modelLocation = glGetUniformLocation(ShaderProgram, "model");
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)(xWindow / yWindow), 0.1f, 100.0f);
        int projectionLocation = glGetUniformLocation(ShaderProgram, "projection");
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

        glm::mat4 view = glm::mat4(1.0f);
        float cameraRadius = 5.0f;
        float xCam = cameraRadius * std::sin(elevation) * std::cos(phi);
        float yCam = cameraRadius * std::cos(elevation);
        float zCam = cameraRadius * std::sin(elevation) * std::sin(phi);
        view = glm::lookAt(glm::vec3(xCam, yCam, zCam), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        int viewLocation = glGetUniformLocation(ShaderProgram, "view");
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

        glm::vec3 cameraPosition = glm::vec3(xCam, yCam, zCam);
        int cameraLocation = glGetUniformLocation(ShaderProgram, "cameraPos");
        glUniform3fv(cameraLocation, 1, &cameraPosition[0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glUniform1i(glGetUniformLocation(ShaderProgram, "SamplesCount"), 16);

        glUniform1f(glGetUniformLocation(ShaderProgram, "metallic"), 0.3);
        glUniform1f(glGetUniformLocation(ShaderProgram, "roughness"), 0.3);

        glm::vec3 goldColor = glm::vec3(1.0f, 0.87451f, 0.0f);
        int matColorLocation = glGetUniformLocation(ShaderProgram, "materialColor");
        glUniform3fv(matColorLocation, 1, &goldColor[0]);

        renderSphere();

        model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

        glm::vec3 stainlessColor = glm::vec3(0.72157f, 0.45098f, 0.20000f);
        glUniform3fv(matColorLocation, 1, &stainlessColor[0]);

        renderSphere();

        glDepthFunc(GL_LEQUAL);  
        glUseProgram(skyShaderProgram); 
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        int skyViewLocation = glGetUniformLocation(skyShaderProgram, "view");
        glUniformMatrix4fv(skyViewLocation, 1, GL_FALSE, &skyView[0][0]);

        int skyProjectionLocation = glGetUniformLocation(skyShaderProgram, "projection");
        glUniformMatrix4fv(skyProjectionLocation, 1, GL_FALSE, &projection[0][0]);

     
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentCubmap);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); 

        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void renderCube()
{

    if (cubeVAO == 0)
    {
        float vertices[] = {

            // Back Face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, 
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, 
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, 

            // Front Face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, 
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, 
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, 
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 

            // Left Face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, 
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, 
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 

            // Right Face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,      
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, 
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, 
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,   

             // Bottom  Face
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
              1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
             -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

             // Top Face
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
              1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
              1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,  
              1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
             -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f        
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int sphereVAO = 0;
unsigned int indexCount;

void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int numOfStacks = 64;
        const unsigned int numOfSections = 64;
        const float pi = 3.14159265359f;
        float radius = 0.5f;

        for (unsigned int x = 0; x <= numOfStacks; ++x)
        {
            for (unsigned int y = 0; y <= numOfSections; ++y)
            {
                float xSegment = (float)x / (float)numOfStacks;
                float ySegment = (float)y / (float)numOfSections;
                float xPos = (std::cos(xSegment * 2.0f * pi) * std::sin(ySegment * pi)) * radius;
                float yPos = (std::cos(ySegment * pi)) * radius;
                float zPos = (std::sin(xSegment * 2.0f * pi) * std::sin(ySegment * pi)) * radius;

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < numOfSections; ++y)
        {
            if (!oddRow) 
            {
                for (unsigned int x = 0; x <= numOfStacks; ++x)
                {
                    indices.push_back(y * (numOfStacks + 1) + x);
                    indices.push_back((y + 1) * (numOfStacks + 1) + x);
                }
            }
            else
            {
                for (int x = numOfStacks; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (numOfStacks + 1) + x);
                    indices.push_back(y * (numOfStacks + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}
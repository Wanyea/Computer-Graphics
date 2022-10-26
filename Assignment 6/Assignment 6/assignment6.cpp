// Assignment 5 - Completed by Wanyea Barbel

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <ext.hpp>
#include <gtx/string_cast.hpp>
#include <gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

const char* srcVS = R"HERE(
	#version 330 core
	layout (location = 0) in vec3 pos; 
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 uvs;

    out vec3 fragNormal;
    out vec2 fragUVs;
    out vec3 fragPos;

    uniform mat4 view;
    uniform mat4 projection;

	void main () 
	{
		  gl_Position = projection * view * vec4(pos, 1);
          fragNormal = vec3(vec4(normalize(normal), 0));
          fragUVs = uvs;
          fragPos = pos;
	}

)HERE";

const char* srcFS = R"HERE(
	#version 330 core

	out vec4 outColor;

    in vec3 fragNormal;
    in vec2 fragUVs;
    in vec3 fragPos; 

    uniform sampler2D tex;
    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 specularColor;

	void main () 
	{
	    vec3 materialColor = texture(tex, fragUVs).rgb; 

        vec3 ambientLight = 0.1 * materialColor;

        vec3 lightDirection = normalize(lightPos - fragPos);
        float diffuseLightVal = max(dot(lightDirection, fragNormal), 0.0);
        vec3 diffuseLight = diffuseLightVal * materialColor;

        vec3 viewDirection = normalize(viewPos - fragPos);
        vec3 HDirection = normalize(lightDirection + viewDirection);
        float specularLightVal = pow(max(dot(fragNormal, HDirection), 0.0), 10.0);
        vec3 specularLight = specularColor * specularLightVal;
    
        outColor = vec4(ambientLight + diffuseLight + specularLight, 1.0);
	}
)HERE";

const char* skyboxSrcVS = R"HERE(
	#version 330 core

    // Positions/Coordinates
    layout (location = 0) in vec3 aPos;
    // Normals (not necessarily normalized)
    layout (location = 1) in vec3 aNormal;
    // Colors
    layout (location = 2) in vec3 aColor;
    // Texture Coordinates
    layout (location = 3) in vec2 aTex;


    // Outputs the current position for the Fragment Shader
    out vec3 crntPos;
    // Outputs the normal for the Fragment Shader
    out vec3 Normal;
    // Outputs the color for the Fragment Shader
    out vec3 color;
    // Outputs the texture coordinates to the Fragment Shader
    out vec2 texCoord;



    // Imports the camera matrix
    uniform mat4 camMatrix;
    // Imports the transformation matrices
    uniform mat4 model;
    uniform mat4 translation;
    uniform mat4 rotation;
    uniform mat4 scale;


    void main()
    {
	    // calculates current position
	    crntPos = vec3(model * translation * rotation * scale * vec4(aPos, 1.0f));
	    // Assigns the normal from the Vertex Data to "Normal"
	    Normal = aNormal;
	    // Assigns the colors from the Vertex Data to "color"
	    color = aColor;
	    // Assigns the texture coordinates from the Vertex Data to "texCoord"
	    texCoord = mat2(0.0, -1.0, 1.0, 0.0) * aTex;
	
	    // Outputs the positions/coordinates of all vertices
	    gl_Position = camMatrix * vec4(crntPos, 1.0);
    }

)HERE";

const char* skyboxSrcFS = R"HERE(
	#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Imports the current position from the Vertex Shader
in vec3 crntPos;
// Imports the normal from the Vertex Shader
in vec3 Normal;
// Imports the color from the Vertex Shader
in vec3 color;
// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;



// Gets the Texture Units from the main function
uniform sampler2D diffuse0;
uniform sampler2D specular0;
// Gets the color of the light from the main function
uniform vec4 lightColor;
// Gets the position of the light from the main function
uniform vec3 lightPos;
// Gets the position of the camera from the main function
uniform vec3 camPos;


vec4 pointLight()
{	
	// used in two variables so I calculate it here to not have to do it twice
	vec3 lightVec = lightPos - crntPos;

	// intensity of light with respect to distance
	float dist = length(lightVec);
	float a = 3.0;
	float b = 0.7;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0f);

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
}

vec4 direcLight()
{
	// ambient lighting
	float ambient = 0.50f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(vec3(1.0f, 1.0f, -2.0f));
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	return (texture(diffuse0, texCoord) * (diffuse + ambient) + texture(specular0, texCoord).r * specular) * lightColor;
}

vec4 spotLight()
{
	// controls how big the area that is lit up is
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	// ambient lighting
	float ambient = 0.20f;

	// diffuse lighting
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(lightPos - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPos - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	float specular = specAmount * specularLight;

	// calculates the intensity of the crntPos based on its angle to the center of the light cone
	float angle = dot(vec3(0.0f, -1.0f, 0.0f), -lightDirection);
	float inten = clamp((angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0f);

	return (texture(diffuse0, texCoord) * (diffuse * inten + ambient) + texture(specular0, texCoord).r * specular * inten) * lightColor;
}


void main()
{
	// outputs final color
	FragColor = direcLight();
}
)HERE";

int xWindow = 700;
int yWindow = 700;

const float PI = 3.1415926535f;

int whichRender = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float camElevation = PI / 2.0f;
float alpha = PI / 2.0f;

float elevation = PI / 2.0f;
float azimuth = PI / 2.0f;

float xLight = 0.0f;
float yLight = 0.0f;

float lightSpeed = 25.0f;

glm::vec3 specularColor = glm::vec3(0.992157, 0.941176, 0.807843);

float skyboxVertices[] =
{
    //   Coordinates
    -1.0f, -1.0f,  1.0f,//        7--------6
     1.0f, -1.0f,  1.0f,//       /|       /|
     1.0f, -1.0f, -1.0f,//      4--------5 |
    -1.0f, -1.0f, -1.0f,//      | |      | |
    -1.0f,  1.0f,  1.0f,//      | 3------|-2
     1.0f,  1.0f,  1.0f,//      |/       |/
     1.0f,  1.0f, -1.0f,//      0--------1
    -1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
    // Right
    1, 2, 6,
    6, 5, 1,
    // Left
    0, 4, 7,
    7, 3, 0,
    // Top
    4, 5, 6,
    6, 7, 4,
    // Bottom
    0, 3, 2,
    2, 1, 0,
    // Back
    0, 1, 5,
    5, 4, 0,
    // Front
    3, 7, 6,
    6, 2, 3
};


float deg2rad(float degree)
{
    return 2 * PI * (degree / 360.0f);
}

static void key_callback(GLFWwindow* window, int key, int keycode, int action, int mods)
{
    float cameraSpeed = 35.0f;
    float speed = 3.0f;
    bool ep = false, em = false, ap = false, am = false;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        whichRender = 0;
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        whichRender = 1;
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        whichRender = 2;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS && elevation < deg2rad(179)) {
        ep = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
        ep = false;
    }

    if (key == GLFW_KEY_W && action == GLFW_PRESS && elevation > deg2rad(1)) {
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
        azimuth += cameraSpeed * speed;
    }

    if (am)
    {
        azimuth -= cameraSpeed * speed;
    }

    if (key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        specularColor = glm::vec3(0.992157, 0.941176, 0.807843); // Brass
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        specularColor = glm::vec3(0.393548, 0.271906, 0.166721); // Bronze
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        specularColor = glm::vec3(0.774957, 0.774957, 0.774957); // Chrome
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        specularColor = glm::vec3(0.256777, 0.137622, 0.086014); // Copper
    }

    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        specularColor = glm::vec3(0.628281, 0.555802, 0.366065); // Gold
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        yLight += lightSpeed * speed;
    }

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        yLight -= lightSpeed * speed;
    }

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        xLight += lightSpeed * speed;
    }

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        xLight -= lightSpeed * speed;
    }

}

int main(void)
{
    // glfw: initialize and configure
// 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int xWindow = 700;
    int yWindow = 700;

    // glfw window creation

    GLFWwindow* window = glfwCreateWindow(xWindow, yWindow, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);


    // Compile vertex shaders

    unsigned int shaderObjVS = glCreateShader(GL_VERTEX_SHADER);
    int lengthVS = (int)strlen(srcVS);
    glShaderSource(shaderObjVS, 1, &srcVS, &lengthVS);
    glCompileShader(shaderObjVS);

    unsigned int skyBoxShaderObjVS = glCreateShader(GL_VERTEX_SHADER);
    int lengthSkyboxVS = (int)strlen(skyboxSrcVS);
    glShaderSource(skyBoxShaderObjVS, 1, &skyboxSrcVS, &lengthSkyboxVS);
    glCompileShader(skyBoxShaderObjVS);

    // Error handling for the vertex shaders

    int success;
    glGetShaderiv(shaderObjVS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(shaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
    }

    glGetShaderiv(skyBoxShaderObjVS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(skyBoxShaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
    }

    // Compile fragment shaders

    unsigned int shaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int lengthFS = (int)strlen(srcFS);
    glShaderSource(shaderObjFS, 1, &srcFS, &lengthFS);
    glCompileShader(shaderObjFS);

    unsigned int skyBoxShaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int lengthSkyboxFS = (int)strlen(skyboxSrcFS);
    glShaderSource(skyBoxShaderObjFS, 1, &skyboxSrcFS, &lengthSkyboxFS);
    glCompileShader(skyBoxShaderObjFS);

    // Error handing for the vertex shaders

    glGetShaderiv(shaderObjFS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(shaderObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
    }

    glGetShaderiv(skyBoxShaderObjFS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(skyBoxShaderObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
    }

    // Link vertex and fragment shader to shader program

    unsigned int ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, shaderObjVS);
    glAttachShader(ShaderProgram, shaderObjFS);
    glAttachShader(ShaderProgram, skyBoxShaderObjVS);
    glAttachShader(ShaderProgram, skyBoxShaderObjFS);

    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);

    // Error handling for shader program

    if (success == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(ShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link shaders:" << std::endl;
        std::cout << message << std::endl;
    }

    glDeleteShader(shaderObjVS);
    glDeleteShader(shaderObjFS);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    std::vector<glm::vec2> uvs;

    const unsigned int numOfStacks = 64;
    const unsigned int numOfSections = 64;
    unsigned int radius = 1.0f;

    for (unsigned int i = 0; i <= numOfStacks; ++i)
    {
        for (unsigned int j = 0; j <= numOfSections; ++j)
        {
            float x = (float)i / (float)numOfStacks;
            float y = (float)j / (float)numOfSections;

            float xPos = radius * (std::cos(x * 2.0f * PI) * std::sin(y * PI));
            float yPos = (std::cos(y * PI)) * radius;
            float zPos = (std::sin(x * 2.0f * PI) * std::sin(y * PI)) * radius;

            positions.emplace_back(xPos, yPos, zPos);
            normals.emplace_back(xPos, yPos, zPos);
            uvs.emplace_back(x, y);
        }
    }

    bool isOddRow = true;

    for (unsigned int y = 0; y < numOfSections; ++y)
    {
        if (isOddRow)
        {
            for (unsigned int x = 0; x <= numOfStacks; ++x)
            {
                indices.push_back((y + 1) * (numOfStacks + 1) + x);
                indices.push_back(y * (numOfStacks + 1) + x);
            }
        }
        else
        {
            for (int x = numOfStacks; x >= 0; --x)
            {
                indices.push_back(y * (numOfStacks + 1) + x);
                indices.push_back((y + 1) * (numOfStacks + 1) + x);
            }
        }

        isOddRow = !isOddRow;
    }

    unsigned int indexCount;
    indexCount = static_cast<unsigned int>(indices.size());

    std::vector<float> data;

    for (unsigned int i = 0, j = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);
        data.push_back(uvs[i].x);
        data.push_back(uvs[i].y);

    }

    unsigned int VAO = 0;
    unsigned int stride = (3 + 3 + 2) * sizeof(float);
    glGenVertexArrays(1, &VAO);
    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    int width, height, channels;
    unsigned char* img = stbi_load("earth_texture.jpg", &width, &height, &channels, 0);
    if (img == NULL) {
        printf("Error: cannot load image.\n");
        exit(1);
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

    glGenerateMipmap(GL_TEXTURE_2D);

    glEnable(GL_CULL_FACE);

    glCullFace(GL_FRONT);

    // Create VAO, VBO, and EBO for the skybox
    unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    // All the faces of the cubemap (make sure they are in this exact order)
    std::string facesCubemap[6] =
    {
        "right.jpg",
        "left.jpg",
        "top.jpg",
        "bottom.jpg",
        "front.jpg",
        "back.jpg"
    };

    // Creates the cubemap texture object
    unsigned int cubemapTexture;
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // These are very important to prevent seams
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // This might help with seams on some systems
    //glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Cycles through all the textures and attaches them to the cubemap object
    for (unsigned int i = 0; i < 6; i++)
    {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(facesCubemap[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D
            (
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                GL_RGB,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load texture: " << facesCubemap[i] << std::endl;
            stbi_image_free(data);
        }
    }


    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (whichRender == 0)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else if (whichRender == 1)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else if (whichRender == 2)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glPointSize(5.0);
        }

        glUseProgram(ShaderProgram);

        glEnable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(ShaderProgram, "tex"), 0);

        glDepthFunc(GL_LEQUAL);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)(xWindow / yWindow), 0.1f, 100.0f);
        int projectionLocation = glGetUniformLocation(ShaderProgram, "projection");
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

        glm::mat4 view = glm::mat4(1.0f);
        float cameraRadius = 5.0f;
        float viewX = cameraRadius * std::sin(camElevation) * std::cos(alpha);
        float viewY = cameraRadius * std::cos(camElevation);
        float viewZ = cameraRadius * std::sin(camElevation) * std::sin(alpha);
        view = glm::lookAt(glm::vec3(viewX, viewY, viewZ), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        int viewLocation = glGetUniformLocation(ShaderProgram, "view");
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

        glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 0.0f);


        lightPos = glm::vec3(viewX, viewY, viewZ);

        std::cout << glm::to_string(lightPos) << std::endl;
        int lightLocation = glGetUniformLocation(ShaderProgram, "lightPos");
        glUniform3f(lightLocation, xLight, yLight, 5.0f);

        int viewPositionLocation = glGetUniformLocation(ShaderProgram, "viewPos");
        glUniform3f(viewPositionLocation, 0.0f, 0.0f, 5.0f);

        int specularColorLocation = glGetUniformLocation(ShaderProgram, "specularColor");
        glUniform3f(specularColorLocation, specularColor.x, specularColor.y, specularColor.z);


        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
// Assignment 6 - Completed by Wanyea Barbel

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
    
    out vec3 Normal;
    out vec3 Position;

    uniform mat4 view;
    uniform mat4 projection;
    uniform mat4 model;

	void main() 
	{
        Normal = normal * mat3(transpose(inverse(model))); 
        Position = vec3(model * vec4(pos, 1.0));
        gl_Position = model * view * projection * vec4(pos, 1,0);
	}

)HERE";

const char* srcFS = R"HERE(
	#version 330 core

	out vec4 FragColor;

    in vec3 Normal;
    in vec3 Position;

    uniform vec3 cameraPosition;
    uniform samplerCube skybox;

    void main()
    {    
        vec3 I = normalize(Position - cameraPosition);
        vec3 R = reflect(I, normalize(Normal));
        FragColor = vec4(texture(skybox, R).rgb, 1.0);
    }
)HERE";

const char* skyboxSrcVS = R"HERE(
	#version 330 core
    layout (location = 0) in vec3 pos;

    out vec3 TexCoords;

    uniform mat4 projection;
    uniform mat4 view;

    void main()
    {
        TexCoords = pos;
        vec4 position = projection * view * vec4(pos, 1.0);
        gl_Position = position.xyww;
    }  
)HERE";

const char* skyboxSrcFS = R"HERE(
	#version 330 core

    out vec4 FragColor;

    in vec3 TexCoords;

    uniform samplerCube skybox;

    void main()
    {    
        FragColor = texture(skybox, TexCoords);
    }
)HERE";

int xWindow = 2000;
int yWindow = 2000;

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

float skyboxVertices[] = {
              
    -5.0f,  5.0f, -5.0f,
    -5.0f, -5.0f, -5.0f,
     5.0f, -5.0f, -5.0f,
     5.0f, -5.0f, -5.0f,
     5.0f,  5.0f, -5.0f,
    -5.0f,  5.0f, -5.0f,

    -5.0f, -5.0f,  5.0f,
    -5.0f, -5.0f, -5.0f,
    -5.0f,  5.0f, -5.0f,
    -5.0f,  5.0f, -5.0f,
    -5.0f,  5.0f,  5.0f,
    -5.0f, -5.0f,  5.0f,

     5.0f, -5.0f, -5.0f,
     5.0f, -5.0f,  5.0f,
     5.0f,  5.0f,  5.0f,
     5.0f,  5.0f,  5.0f,
     5.0f,  5.0f, -5.0f,
     5.0f, -5.0f, -5.0f,

    -5.0f, -5.0f,  5.0f,
    -5.0f,  5.0f,  5.0f,
     5.0f,  5.0f,  5.0f,
     5.0f,  5.0f,  5.0f,
     5.0f, -5.0f,  5.0f,
    -5.0f, -5.0f,  5.0f,

    -5.0f,  5.0f, -5.0f,
     5.0f,  5.0f, -5.0f,
     5.0f,  5.0f,  5.0f,
     5.0f,  5.0f,  5.0f,
    -5.0f,  5.0f,  5.0f,
    -5.0f,  5.0f, -5.0f,

    -5.0f, -5.0f, -5.0f,
    -5.0f, -5.0f,  5.0f,
     5.0f, -5.0f, -5.0f,
     5.0f, -5.0f, -5.0f,
    -5.0f, -5.0f,  5.0f,
     5.0f, -5.0f,  5.0f
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

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int xWindow = 1000;
    int yWindow = 1000;

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



    // Error handling for the vertex shaders

    int success;
    glGetShaderiv(shaderObjVS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(shaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
    }


    // Compile fragment shaders

    unsigned int shaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int lengthFS = (int)strlen(srcFS);
    glShaderSource(shaderObjFS, 1, &srcFS, &lengthFS);
    glCompileShader(shaderObjFS);


    // Error handing for the vertex shaders

    glGetShaderiv(shaderObjFS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(shaderObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Fragment Shader" << std::endl;
        std::cout << message << std::endl;
    }

    // Link vertex and fragment shader to shader program

    unsigned int ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, shaderObjVS);
    glAttachShader(ShaderProgram, shaderObjFS);

    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);

    glValidateProgram(ShaderProgram);

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

    // Skybox VS, FS, Shader Program

    unsigned int skyShaderObjVS = glCreateShader(GL_VERTEX_SHADER);
    int skyVertexLength = (int)strlen(skyboxSrcVS);
    glShaderSource(skyShaderObjVS, 1, &skyboxSrcVS, &skyVertexLength);
    glCompileShader(skyShaderObjVS);
    int skyVertexSuccess;
    glGetShaderiv(skyShaderObjVS, GL_COMPILE_STATUS, &skyVertexSuccess);

    if (skyVertexSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(skyShaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Sky Vertex Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(skyShaderObjVS);
    }

    unsigned int skyShaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int skyFragmentLength = (int)strlen(skyboxSrcFS);
    glShaderSource(skyShaderObjFS, 1, &skyboxSrcFS, &skyFragmentLength);
    glCompileShader(skyShaderObjFS);
    int skyFragmentSuccess;
    glGetShaderiv(skyShaderObjFS, GL_COMPILE_STATUS, &skyFragmentSuccess);

    if (skyFragmentSuccess == GL_FALSE) {
        char message[1024];
        glGetShaderInfoLog(skyShaderObjFS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Sky Fragment Shader" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(skyShaderObjFS);
    }

    unsigned int skyboxShaderProgram = glCreateProgram();
    glAttachShader(skyboxShaderProgram, skyShaderObjVS);
    glAttachShader(skyboxShaderProgram, skyShaderObjFS);
    glLinkProgram(skyboxShaderProgram);

    int skyboxShaderSuccess;
    glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &skyboxShaderSuccess);

    if (skyboxShaderSuccess == GL_FALSE) {
        char message[1024];
        glGetProgramInfoLog(skyboxShaderProgram,
            sizeof(message), NULL, message);
        std::cout << "Failed to link sky shaders:"
            << std::endl;
        std::cout << message << std::endl;
    }

    glValidateProgram(skyboxShaderProgram);


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

    int width, height, channels;

    // Create VAO and VBO for the skybox.
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    std::vector<std::string> skyboxFaces =
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

    for (unsigned int i = 0; i < skyboxFaces.size(); i++)
    {
        unsigned char* faceData = stbi_load(skyboxFaces[i].c_str(), &width, &height, &channels, 0);
        if (faceData)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, faceData);
            stbi_image_free(faceData);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << skyboxFaces[i] << std::endl;
            stbi_image_free(faceData);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glUseProgram(ShaderProgram);
    glUniform1i(glGetUniformLocation(ShaderProgram, "skybox"), 0);

    glUseProgram(skyboxShaderProgram);
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "skybox"), 0);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        glm::mat4 model = glm::mat4(1.0f);
        int location = glGetUniformLocation(ShaderProgram, "model");
        glUniformMatrix4fv(location, 1, GL_FALSE, &model[0][0]);

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

        glm::vec3 cameraPosition = glm::vec3(viewX, viewY, viewZ);
        int cameraLocation = glGetUniformLocation(ShaderProgram, "cameraPosition");
        glUniform3fv(cameraLocation, 1, &cameraPosition[0]);

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glUniform1i(glGetUniformLocation(ShaderProgram, "skybox"), 0);
        glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthFunc(GL_LEQUAL); 
        glUseProgram(skyboxShaderProgram); 
        int skyViewLocation = glGetUniformLocation(skyboxShaderProgram, "view");
        glUniformMatrix4fv(skyViewLocation, 1, GL_FALSE, &view[0][0]);

        int skyProjectionLocation = glGetUniformLocation(skyboxShaderProgram, "projection");
        glUniformMatrix4fv(skyProjectionLocation, 1, GL_FALSE, &projection[0][0]);

 
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
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


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
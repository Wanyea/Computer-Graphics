#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <utility> 
#include <stdexcept> 
#include <sstream> 
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>



void renderCube();
void renderSphere();


const char* srcVS = R"HERE(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 fragNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    fragNormal = vec3(vec4(normalize(normal), 0));
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)HERE";

const char* srcFS = R"HERE(
#version 330 core

out vec4 outColor;
in vec3 fragNormal;

uniform vec3 RGB;

void main () 
{
    outColor = vec4(RGB, 1);
}
)HERE";


const float pi = 3.1415926535f;
int whichKeyPressed = 0;
float elevation = pi / 2.0f;
float phi = pi / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int xWindow = 1300;
int yWindow = 1300;


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
    float D65[81] = 
    {   
        49.9755f, 52.3118f, 54.6482f, 68.7015f, 82.7549f, 87.1204f, 91.486f, 
        92.4589f, 93.4318f, 90.057f, 86.6823f, 95.7736f, 104.865f, 110.936f, 117.008f, 117.41f,
        117.812f, 116.336f, 114.861f, 115.392f, 115.923f, 112.367f, 108.811f, 109.082f,
        109.354f, 108.578f, 107.802f, 106.296f, 104.79f, 106.239f, 107.689f, 106.047f,
        104.405f, 104.225f, 104.046f, 102.023f, 100.0f, 98.1671f, 96.3342f, 96.0611f,
        95.788f, 92.2368f, 88.6856f, 89.3459f, 90.0062f, 89.8026f, 89.5991f, 88.6489f,
        87.6987f, 85.4936f, 83.2886f, 83.4939f, 83.6992f, 81.863f, 80.0268f, 80.1207f,
        80.2146f, 81.2462f, 82.2778f, 80.281f, 78.2842f, 74.0027f, 69.7213f, 70.6652f,
        71.6091f, 72.979f, 74.349f, 67.9765f, 61.604f, 65.7448f, 69.8856f, 72.4863f,
        75.087f, 69.3398f, 63.5927f, 55.0054f, 46.4182f, 56.6118f, 66.8054f, 65.0941f, 
        63.3828f 
    };

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
        }
        else {
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
    std::vector<glm::vec3> normalPoints;
    std::vector<glm::vec3> tangentPoints;
    std::vector<glm::vec3> bitangentPoints;

    for (unsigned int i = 0, j = 0; i < positions.size(); ++i)
    {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);

        normalPoints.push_back(positions[i]);
        normalPoints.emplace_back(1.0f, 0.0f, 0.0f);
        normalPoints.push_back(positions[i] + (positions[i] * 0.1f));
        normalPoints.emplace_back(1.0f, 0.0f, 0.0f);

        glm::vec3 tangentVar = glm::normalize(glm::cross(positions[i], glm::vec3(0.0f, 0.0f, 1.0f)));

        tangentPoints.push_back(positions[i]);
        tangentPoints.emplace_back(0.0f, 0.0f, 1.0f);
        tangentPoints.push_back(positions[i] + 0.1f * tangentVar);
        tangentPoints.emplace_back(0.0f, 0.0f, 1.0f);

        glm::vec3 bitangentVar = glm::normalize(glm::cross(positions[i], tangentVar));

        bitangentPoints.push_back(positions[i]);
        bitangentPoints.emplace_back(0.0f, 1.0f, 0.0f);
        bitangentPoints.push_back(positions[i] + 0.1f * bitangentVar);
        bitangentPoints.emplace_back(0.0f, 1.0f, 0.0f);

    }

    unsigned int sphereVAO = 0;
    glGenVertexArrays(1, &sphereVAO);
    unsigned int vbo, ebo;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(glm::vec3), &data[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);
    unsigned int stride = (1 + 1) * sizeof(glm::vec3);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    unsigned int normalVAO = 0;
    glGenVertexArrays(1, &normalVAO);
    unsigned int vboNormal;
    glGenBuffers(1, &vboNormal);
    glBindVertexArray(normalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER, normalPoints.size() * sizeof(glm::vec3), &normalPoints[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    unsigned int tangentVAO = 0;
    glGenVertexArrays(1, &tangentVAO);
    unsigned int vboTangent;
    glGenBuffers(1, &vboTangent);
    glBindVertexArray(tangentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vboTangent);
    glBufferData(GL_ARRAY_BUFFER, tangentPoints.size() * sizeof(glm::vec3), &tangentPoints[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    unsigned int bitangentVAO = 0;
    glGenVertexArrays(1, &bitangentVAO);
    unsigned int vboBitangent;
    glGenBuffers(1, &vboBitangent);
    glBindVertexArray(bitangentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vboBitangent);
    glBufferData(GL_ARRAY_BUFFER, bitangentPoints.size() * sizeof(glm::vec3), &bitangentPoints[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    glm::mat3 transformationMatrix = glm::mat3(3.2404542f, -1.5371385f, -0.4985314f, -0.9692660f, 1.8760108f, 0.0415560f, 0.0556434f, -0.2040259f, 1.0572252f);
    transformationMatrix = glm::transpose(transformationMatrix);

    std::cout << transformationMatrix[0][0];
    printf(" ");
    std::cout << transformationMatrix[2][0];
    printf("\n");

    std::ifstream xyzCSV("XYZ.csv");
    if (!xyzCSV.is_open()) throw std::runtime_error("There was an error opening the CSV...");

    std::vector<glm::vec3> xyzVals;

    std::string line, colname;
    float val;

    while (std::getline(xyzCSV, line))
    {
        std::stringstream stringStream(line);

        int xyzIndex = 0;
        glm::vec3 currCMF;

        // Grab each x,y,z from the CSV and assignment to the proper index. 
        while (stringStream >> val) {
            if (stringStream.peek() == ',') stringStream.ignore();

            if (xyzIndex == 0)
            {
                currCMF.x = val;
            }
            if (xyzIndex == 1)
            {
                currCMF.y = val;
            }
            if (xyzIndex == 2)
            {
                currCMF.z = val;
            }

            xyzIndex++;
        }
        xyzVals.push_back(currCMF);
    }

    // Open CSV file and parse each line 
    std::ifstream specCSV("MacbethColorChecker.csv");
    if (!specCSV.is_open()) throw std::runtime_error("There was an error opening the CSV...");

    std::vector<float> specVals[24];

    std::string lineSpec, colnameSpec;
    float valSpec;

    while (std::getline(specCSV, lineSpec))
    {
        std::stringstream stringStream(lineSpec);

        int xyzIndex = 0;

        while (stringStream >> valSpec) {

            specVals[xyzIndex].push_back(valSpec);
            if (stringStream.peek() == ',') stringStream.ignore();


            xyzIndex++;
        }
    }

    float chromaticity[9] = { 0.67f, 0.33f, 0.0f, 0.21f, 0.71f, 0.08f, 0.15f, 0.06f, 0.79f };
    float currentD65[3] = { 0.3127f, 0.3291f, 0.3582f };

    glm::mat3 chromatrix = glm::make_mat3(chromaticity);
    chromatrix = glm::transpose(chromatrix);
    glm::mat3 chromatrixInverse = glm::inverse(chromatrix);
    glm::vec3 white = glm::make_vec3(currentD65);
    glm::mat3 RGBTransformer = glm::mat3(0.0f);
    glm::vec3 scaler = glm::vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 3; i++)
    {
        scaler[i] = (chromatrixInverse[i][0] * white[0]) + (chromatrixInverse[i][1] * white[1]) + (chromatrixInverse[i][2] * white[2]);
    }

    for (int i = 0; i < 3; i++)
    {
        RGBTransformer[i][0] = chromatrixInverse[i][0] / scaler.x;
        RGBTransformer[i][1] = chromatrixInverse[i][1] / scaler.y;
        RGBTransformer[i][2] = chromatrixInverse[i][2] / scaler.z;

    }
    std::cout << RGBTransformer[0][0];
    printf("\n");


    std::vector<glm::vec3> rgbValues;

    for (int i = 0; i < 24; i++)
    {
        float Ysum = 0.0f;
        glm::vec3 currSpec = glm::vec3(0.0);
        for (int j = 0; j < specVals[0].size(); j++)
        {
            currSpec.x += specVals[i][j] * xyzVals[j].x * D65[j];
            currSpec.y += specVals[i][j] * xyzVals[j].y * D65[j];
            currSpec.z += specVals[i][j] * xyzVals[j].z * D65[j];

            Ysum += (D65[j] * xyzVals[j].y);


        }
        currSpec.x /= Ysum;
        currSpec.y /= Ysum;
        currSpec.z /= Ysum;


        glm::vec3 calculatedRGB = transformationMatrix * currSpec;
        calculatedRGB.x = transformationMatrix[0].x * currSpec.x + transformationMatrix[1].x * currSpec.y + transformationMatrix[2].x * currSpec.z;
        calculatedRGB.y = transformationMatrix[0].y * currSpec.x + transformationMatrix[1].y * currSpec.y + transformationMatrix[2].y * currSpec.z;
        calculatedRGB.z = transformationMatrix[0].z * currSpec.x + transformationMatrix[1].z * currSpec.y + transformationMatrix[2].z * currSpec.z;

        if (calculatedRGB.x < 0 || calculatedRGB.y < 0 || calculatedRGB.z < 0)
        {
            float smallest = 99999.0f;

            if (calculatedRGB.x < smallest)
                smallest = calculatedRGB.x;
            if (calculatedRGB.y < smallest)
                smallest = calculatedRGB.y;
            if (calculatedRGB.z < smallest)
                smallest = calculatedRGB.z;

            smallest = -1.0 * smallest;

            calculatedRGB.x += smallest;
            calculatedRGB.y += smallest;
            calculatedRGB.z += smallest;
        }

        if (calculatedRGB.x < 0.0031308f)
        {
            calculatedRGB.x *= 12.92f;
        }
        else
        {
            calculatedRGB.x = (1.055f * (std::pow(calculatedRGB.x, (1.0f / 2.4f))) - 0.055f);
        }

        if (calculatedRGB.y < 0.0031308f)
        {
            calculatedRGB.y *= 12.92f;
        }
        else
        {
            calculatedRGB.y = (1.055f * (std::pow(calculatedRGB.y, (1.0f / 2.4f))) - 0.055f);
        }

        if (calculatedRGB.z < 0.0031308f)
        {
            calculatedRGB.z *= 12.92f;
        }
        else
        {
            calculatedRGB.z = (1.055f * (std::pow(calculatedRGB.z, (1.0f / 2.4f))) - 0.055f);
        }

        rgbValues.push_back(calculatedRGB);
    }

        // Loop until the window is closed. 
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

            glfwSwapBuffers(window);
        }

        glUseProgram(ShaderProgram);

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

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        model = glm::translate(model, glm::vec3(-7.5f, 5.0f, 0.0f));
        int modelLocation = glGetUniformLocation(ShaderProgram, "model");

        int rgbIndex = 0;
        int rgbLocation = glGetUniformLocation(ShaderProgram, "RGB");

        // Render all 24 cubes into the scene. 
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
                glUniform3fv(rgbLocation, 1, &rgbValues[rgbIndex][0]);
                rgbIndex++;

                renderCube();
                model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));
            }
            model = glm::translate(model, glm::vec3(-18.0f, -3.0f, 0.0f));
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

#include <iostream>
#include <vector>
#include <math.h>
#include <glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const char* srcVS = R"HERE(
	#version 330 core
	layout (location = 0) in vec3 pos; 
    layout (location = 1) in vec3 normal;
 
    out vec3 fragNormal;

    uniform mat4 view;
    uniform mat4 projection;

	void main () 
	{
		  gl_Position = projection * view * vec4(pos, 1);
          fragNormal = vec3(vec4(normalize(normal), 0));
	}

)HERE";

const char* srcFS = R"HERE(
	#version 330 core
	out vec4 FragColor;
    in vec3 fragNormal;

	void main () 
	{
	   FragColor = vec4(abs(fragNormal), 1.0);
	}
)HERE";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int xWindow = 700;
    int yWindow = 700;

    const float PI = 3.14159265359f;
    int whichRender = 0;
    float elevation = PI / 2.0f;
    float alpha = PI / 2.0f;

    // glfw window creation
    // --------------------
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

    // Compile vertex shader

    unsigned int shaderObjVS = glCreateShader(GL_VERTEX_SHADER);
    int lengthVS = (int)strlen(srcVS);
    glShaderSource(shaderObjVS, 1, &srcVS, &lengthVS);
    glCompileShader(shaderObjVS);

    // Error handling for the vertex shader

    int success;
    glGetShaderiv(shaderObjVS, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[1024];
        glGetShaderInfoLog(shaderObjVS, sizeof(message), NULL, message);
        std::cout << "Failed to compile: Vertex Shader" << std::endl;
        std::cout << message << std::endl;
    }

    // Compile fragment shader

    unsigned int shaderObjFS = glCreateShader(GL_FRAGMENT_SHADER);
    int lengthFS = (int)strlen(srcFS);
    glShaderSource(shaderObjFS, 1, &srcFS, &lengthFS);
    glCompileShader(shaderObjFS);

    // Error handing for the vertex shader

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

    unsigned int index;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;


    unsigned int stride = (3 + 3) * sizeof(float);

    const unsigned int numOfStacks = 64;
    const unsigned int numOfSectors = 64;
    unsigned int radius = 1.0f;

    for (unsigned int i = 0; i <= numOfStacks; ++i)
    {
        for (unsigned int j = 0; j <= numOfSectors; ++j)
        {
            float x = ((float)i) / ((float)numOfStacks);
            float y = ((float)j) / ((float)numOfSectors);

            float xPos = radius * (std::cos(x * 2.0f * PI) * std::sin(y * PI));
            float yPos = radius * (std::cos(y * PI));
            float zPos = radius * (std::sin(x * 2.0f * PI) * std::sin(y * PI));

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool isOddRow = true;

    for (unsigned int y = 0; y < numOfSectors; ++y)
    {
        if (isOddRow)
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

        isOddRow = !isOddRow;
    }

    index = static_cast<unsigned int>(indices.size());

    std::vector<float> dataPoints;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
    std::vector<glm::vec3> normals2;

    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        dataPoints.push_back(positions[i].x);
        dataPoints.push_back(positions[i].y);
        dataPoints.push_back(positions[i].z);

        if (normals2.size() > 0)
        {
            dataPoints.push_back(normals2[i].x);
            dataPoints.push_back(normals2[i].y);
            dataPoints.push_back(normals2[i].z);
        }

        normals2.push_back(positions[i]);
        normals2.emplace_back(1.0f, 0.0f, 0.0f);
        normals2.push_back(positions[i] + (positions[i] * 0.1f));
        normals2.emplace_back(1.0f, 0.0f, 0.0f);

        glm::vec3 normCross = glm::normalize(glm::cross(positions[i], glm::vec3(0.0f, 0.0f, 1.0f)));
        tangents.push_back(positions[i]);
        tangents.emplace_back(0.0f, 0.0f, 1.0f);
        tangents.push_back(positions[i] + 0.1f * normCross);
        tangents.emplace_back(0.0f, 0.0f, 1.0f);

        glm::vec3 normalCross = glm::normalize(glm::cross(positions[i], normCross));
        bitangents.push_back(positions[i]);
        bitangents.emplace_back(0.0f, 1.0f, 0.0f);
        bitangents.push_back(positions[i] + 0.1f * normalCross);
        bitangents.emplace_back(0.0f, 1.0f, 0.0f);

    }

    unsigned int VAO[4], VBO[4], EBO[1];


    // Sphere
    glGenVertexArrays(1, &VAO[0]);
    glGenBuffers(1, &VBO[0]);
    glGenBuffers(1, &EBO[0]);
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, dataPoints.size() * sizeof(glm::vec3), &dataPoints[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    // Normals
    glGenVertexArrays(1, &VAO[1]);
    glGenBuffers(1, &VBO[1]);
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, normals2.size() * sizeof(glm::vec3), &normals2[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));

    // Tangents 
    glGenVertexArrays(1, &VAO[2]);
    glGenBuffers(1, &VBO[2]);
    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(glm::vec3), &tangents[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));


    glGenVertexArrays(1, &VAO[3]);
    glGenBuffers(1, &VBO[3]);
    glBindVertexArray(VAO[3]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, bitangents.size() * sizeof(glm::vec3), &bitangents[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec3)));



    while (!glfwWindowShouldClose(window))
    {

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);  

        // Poll for and process events
        glfwPollEvents();

        glUseProgram(ShaderProgram);

        // Create projection 
        glm::mat4 projection = glm::perspective(glm::radians(30.0f), (float)(xWindow / yWindow), 0.1f, 50.0f);
        int projectionLocation = glGetUniformLocation(ShaderProgram, "projection");
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);

        glm::mat4 view = glm::mat4(1.0f);
        float radOfCamera = 6.0f;
        float x = radOfCamera * std::sin(elevation) * std::cos(alpha);
        float y = radOfCamera * std::cos(elevation);
        float z = radOfCamera * std::sin(elevation) * std::sin(alpha);

        view = glm::lookAt(glm::vec3(x, y, z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        int viewLocation = glGetUniformLocation(ShaderProgram, "view");
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

        // Draw calls:

        //1. Sphere
        glBindVertexArray(VAO[0]);
        glDrawElements(GL_TRIANGLE_STRIP, index, GL_UNSIGNED_INT, 0);

        //2. Tangent Line
        glBindVertexArray(VAO[2]);
        glDrawArrays(GL_LINES, 0, tangents.size() / 2);

        //3. Bitangent Line
        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_LINES, 0, bitangents.size() / 2);

        //4. Normal Line
        glBindVertexArray(VAO[1]);
        glDrawArrays(GL_LINES, 0, normals2.size() / 2);

        glBindVertexArray(0);
        glUseProgram(0);

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }



    glfwTerminate();
    return 0;
}




void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(15.0);
    }
    else if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }


}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}








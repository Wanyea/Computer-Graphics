#include <iostream>
#include <vector>
#include <math.h>
#include <glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height); 
void processInput(GLFWwindow* window);
void renderSphere();

const char* srcVS = R"HERE(
	#version 330 core
	layout (location = 0) in vec3 aPos; 
    layout (location = 1) in vec3 normal;
 
    out vec3 fragNormal;

	void main () 
	{
		  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); 
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


    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
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

    //float radius = 0.2f;
    //const float PI = 3.14159265359f;
    //const float sectorCount = 64.0f;
    //const float stackCount = 64.0f;

    //float x, y, z, xy;                              // vertex position
    //float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal

    //float sectorStep = 2 * PI / sectorCount;
    //float stackStep = PI / stackCount;
    //float sectorAngle, stackAngle;

    //std::vector<float> vertices;
    //std::vector<float> normals;
    //std::vector<float> t;

    //for (int i = 0; i <= stackCount; ++i)
    //{
    //    stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
    //    xy = radius * cosf(stackAngle);             // r * cos(u)
    //    z = radius * sinf(stackAngle);              // r * sin(u)

    //    // add (sectorCount+1) vertices per stack
    //    // the first and last vertices have same position and normal, but different tex coords
    //    for (int j = 0; j <= sectorCount; ++j)
    //    {
    //        sectorAngle = j * sectorStep;           // starting from 0 to 2pi

    //        // vertex position (x, y, z)
    //        x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
    //        y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
    //   
    //        vertices.push_back(x);
    //        vertices.push_back(y);
    //        vertices.push_back(z);

    //        std::cout << x << std::endl;

    //        // normalized vertex normal (nx, ny, nz)
    //        nx = x * lengthInv;
    //        ny = y * lengthInv;
    //        nz = z * lengthInv;
    //        normals.push_back(nx);
    //        normals.push_back(ny);
    //        normals.push_back(nz);


    //    }
    //}


    //std::vector<unsigned int> indices;

    //float k1, k2;
    //for (int i = 0; i < stackCount; ++i)
    //{
    //    k1 = i * (sectorCount + 1);     // beginning of current stack
    //    k2 = k1 + sectorCount + 1;      // beginning of next stack

    //    for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
    //    {
    //        // 2 triangles per sector excluding first and last stacks
    //        // k1 => k2 => k1+1
    //        if (i != 0)
    //        {
    //            indices.push_back(k1);
    //            indices.push_back(k2);
    //            indices.push_back(k1 + 1);
    //        }

    //        // k1+1 => k2 => k2+1
    //        if (i != (stackCount - 1))
    //        {
    //            indices.push_back(k1 + 1);
    //            indices.push_back(k2);
    //            indices.push_back(k2 + 1);
    //        }

    //        std::cout << k1 << std::endl;

    //    }
    //}

    // Create vertices & normals

   /* unsigned int numX = 64.0f;
    unsigned int numY = 64.0f;
    const float PI = 3.14159265359f;
    unsigned int radius = 1.0f;
    glm::vec3 position;
    glm::vec3 normal;

    for(unsigned int x = 0; x <= numX; ++x)
    {
        for (unsigned int y = 0; y <= numY; ++y)
        {
            float xI = (float)x / (float)numX;
            float yI = (float)y / (float)numY;

            float theta = xI * 2.0f * PI;
            float phi = yI * PI;

            float xPos = radius * std::cos(theta) * std::sin(phi);
            float yPos = radius * std::cos(phi);
            float zPos = radius * std::sin(theta) * std::sin(phi);

            glm::vec3 position = glm::vec3(xPos, yPos, zPos);
            glm::vec3 normal = glm::vec3(xPos, yPos, zPos) / radius;

        }
    }*/

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


 
        

        //glBindVertexArray(VAO[1]);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(t), &t, GL_STATIC_DRAW);
        //glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);	// Vertex attributes stay the same
        //glEnableVertexAttribArray(2);

        //glBindVertexArray(VAO[2]);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);	// Vertex attributes stay the same
        //glEnableVertexAttribArray(0);

        //glBindVertexArray(VAO[3]);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);	// Vertex attributes stay the same
        //glEnableVertexAttribArray(0);



       // glBindVertexArray(0);



        while (!glfwWindowShouldClose(window))
        {

            processInput(window);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(ShaderProgram);
           
            renderSphere();

            glUseProgram(0);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }



        glfwTerminate();
        return 0;
}




void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void renderSphere()
{


    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 64;
    const unsigned int Y_SEGMENTS = 64;
    const float PI = 3.14159265359f;
    for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
    {
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            positions.push_back(glm::vec3(xPos, yPos, zPos));
            normals.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    bool oddRow = false;
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
    {
        if (!oddRow) // even rows: y == 0, y == 2; and so on
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                indices.push_back(y * (X_SEGMENTS + 1) + x);
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            }
        }
        else
        {
            for (int x = X_SEGMENTS; x >= 0; --x)
            {
                indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                indices.push_back(y * (X_SEGMENTS + 1) + x);
            }
        }
        oddRow = !oddRow;
    }
    unsigned int indexCount = static_cast<unsigned int>(indices.size());

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


        unsigned int VAO[4], VBO[3], EBO[2];
        unsigned int stride = (3 + 2 + 3) * sizeof(float);

        glGenVertexArrays(4, VAO);
        glGenBuffers(2, VBO);
        glGenBuffers(2, EBO);

        glBindVertexArray(VAO[0]);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

        glBindVertexArray(VAO[0]);

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }
}


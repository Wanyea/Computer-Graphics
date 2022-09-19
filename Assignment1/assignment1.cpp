#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const char* srcVS = R"HERE(
	#version 330 core

	layout (location = 0) in vec3 aPos; 

	void main () 
	{
		  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); 
	}
)HERE";

const char* srcFS = R"HERE(
	#version 330 core

	out vec4 FragColor;

	void main () 
	{
	   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
	}
)HERE";

#define fillMode = GL_FILL;

int main()
{
	// ** Render Window - Assignment 0 **

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	// ** Render Triangle - Assignment 1 **

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

	float positions[] = {
		-0.5f, -0.5f, 0.0f, // left
		 0.5f, -0.5f, 0.0f, // right
		 0.0f,  0.5f, 0.0f,  // top

		 0.5f, -0.5f, 0.0f, // right
		 1.0f,  0.5f, 0.0f, // top-right
		 0.0f, 0.5f, 0.0f, // top 

		 -1.0f,  0.5f, 0.0f, // top-left
		 -0.5f, -0.5f, 0.0f, // left
		 0.0f, 0.5f, 0.0f // top
		 
		 

	};
	 
	unsigned int vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw triangle

		glUseProgram(ShaderProgram);
		glBindVertexArray(vao);

		glDrawArrays(GL_TRIANGLES, 0, 9);

		glUseProgram(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
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

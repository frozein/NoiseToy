#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

//--------------------------------------------------------------------------------------------------------------------------------//
//GLOBALS:

#define WORK_GROUP_SIZE 16

GLuint g_windowW = 800;
GLuint g_windowH = 800;

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY FUNCTIONS:

int compile_shader(const char* path, GLenum type);

//--------------------------------------------------------------------------------------------------------------------------------//
//CALLBACK FUNCTIONS:

void GLAPIENTRY gl_message_callback       (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
void            framebuffer_size_callback (GLFWwindow* window, int w, int h);
void            key_callback              (GLFWwindow* window, int key, int scancode, int action, int mods);

//--------------------------------------------------------------------------------------------------------------------------------//
//MAIN: 

int main()
{
	//init GLFW:
	//---------------------------------
	if(glfwInit() == GL_FALSE)
	{
		printf("FAILED TO INITIALIZE GLFW\n");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

	//create and init window:
	//---------------------------------
	GLFWwindow* window = glfwCreateWindow(g_windowW, g_windowH, "NoiseToy", NULL, NULL);
	if(window == NULL)
	{
		printf("ERROR - FAILED TO CREATE WINDOW\n");
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	//load opengl functions:
	//---------------------------------
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("ERROR - FAILED TO INITIALIZE GLAD\n");
		return -1;
	}

	//set gl viewport:
	//---------------------------------
	glViewport(0, 0, g_windowW, g_windowH);

	//set callback functions:
	//---------------------------------
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_message_callback, 0);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	//init dear imgui:
	//---------------------------------
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
	ImGui::StyleColorsDark();

	float scaleX, scaleY;
	glfwGetWindowContentScale(window, &scaleX, &scaleY);
	io.FontGlobalScale = scaleX;

	//compile shaders:
	//---------------------------------

	//fractal shader:
	int noiseShader = compile_shader("shaders/noise.comp", GL_COMPUTE_SHADER);
	if(noiseShader < 0)
	{
		printf("ERROR - COULD NOT COMPILE NOISE SHADER");
		return -1;
	}

	GLuint noiseProgram = glCreateProgram();
	glAttachShader(noiseProgram, noiseShader);
	glLinkProgram(noiseProgram);

	GLint linkSuccess;
	glGetProgramiv(noiseProgram, GL_LINK_STATUS, &linkSuccess);
	if(!linkSuccess)
	{
		printf("ERROR - COULD NOT LINK NOISE PROGRAM\n");
		return -1;
	}

	glDeleteShader(noiseShader);

	//quad shader:
	int vertShader = compile_shader("shaders/quad.vert", GL_VERTEX_SHADER);
	int fragShader = compile_shader("shaders/quad.frag", GL_FRAGMENT_SHADER);
	if(vertShader < 0 || fragShader < 0)
	{
		printf("ERROR - COULD NOT COMPILE QUAD SHADERS");
		return -1;
	}

	GLuint quadProgram = glCreateProgram();
	glAttachShader(quadProgram, vertShader);
	glAttachShader(quadProgram, fragShader);
	glLinkProgram(quadProgram);

	glGetProgramiv(quadProgram, GL_LINK_STATUS, &linkSuccess);
	if(!linkSuccess)
	{
		printf("ERROR - COULD NOT LINK QUAD PROGRAM\n");
		return -1;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	//generate quad buffer:
	//---------------------------------
	GLfloat quadVertices[] = 
	{
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
	};
	GLuint quadIndices[] = 
	{
		0, 1, 3,
		1, 2, 3
	};

	GLuint quadVAO, VBO, EBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(quadVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		printf("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER\n");
		return -1;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	if(glGetError() == GL_OUT_OF_MEMORY)
	{
		printf("ERROR - FAILED TO GENERATE FINAL QUAD BUFFER\n");
		return -1;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(long long)(3 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(1);

	//generate texture:
	//---------------------------------
	GLuint textureW = 128;
	GLuint textureH = 128;

	GLuint lastTextureW = textureW; //for determining if texture needs to be resized
	GLuint lastTextureH = textureH;

	GLuint noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureW, textureH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//main loop:
	//---------------------------------
	float lastFrame = (float)glfwGetTime();

	while(!glfwWindowShouldClose(window))
	{
		//find deltatime:
		float currentTime = (float)glfwGetTime();
		float deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;

		//dispatch compute shader:
		glUseProgram(noiseProgram);

		glBindImageTexture(0, noiseTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		
		glDispatchCompute((GLuint)ceilf((float)textureW / WORK_GROUP_SIZE), (GLuint)ceilf((float)textureH / WORK_GROUP_SIZE), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//run final quad shader:
		float screenAspect = (float)g_windowW / g_windowH;
		float textureAspect = (float)textureW / textureH;

		float scale[2];
		scale[0] = screenAspect > textureAspect ? textureAspect / screenAspect : 1.0f;
		scale[1] = screenAspect < textureAspect ? screenAspect / textureAspect : 1.0f;

		glUseProgram(quadProgram);

		glUniform2fv(glGetUniformLocation(quadProgram, "scale"), 1, scale);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));

		//render gui:
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//...

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//resize texture if needed:
		if(textureW != lastTextureW || textureH != lastTextureH)
		{
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureW, textureH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			lastTextureW = textureW;
			lastTextureH = textureH;
		}

		//swap buffers and poll events:
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and return:
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteProgram(quadProgram);
	glDeleteProgram(noiseProgram);
	glfwTerminate();

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------//

int compile_shader(const char* path, GLenum type)
{
	//load into a buffer:
	//---------------------------------
	char* source = 0;
	long length;
	FILE* file = fopen(path, "rb");

	if(!file)
	{
		printf("ERROR - COULD NOT OPEN FILE %s\n", path);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	source = (char*)malloc(length + 1);

	if(!source)
	{
		printf("ERROR - COULD NOT ALLOCATE MEMORY FOR SHADER SOURCE CODE");
		fclose(file);
		return -1;
	}

	fread(source, length, 1, file);
	source[length] = '\0';

	fclose(file);

	//compile:
	//---------------------------------
	unsigned int shader;
	int success;

	shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	free(source);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		GLsizei logLength;
		char message[1024];
		glGetShaderInfoLog(shader, 1024, &logLength, message);
		printf("\n\nINFO LOG - %s%s\n\n\n", message, path);

		glDeleteShader(shader);
		return -1;
	}

	return shader;
}

//--------------------------------------------------------------------------------------------------------------------------------//

void GLAPIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if(severity == GL_DEBUG_SEVERITY_NOTIFICATION || type == 0x8250)
		return;

	printf("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		  (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		   type, severity, message);
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
	g_windowW = w;
	g_windowH = h;
	glViewport(0, 0, g_windowW, g_windowH);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)  
		glfwSetWindowShouldClose(window, true);
}
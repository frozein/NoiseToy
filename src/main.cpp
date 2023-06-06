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

#define WORK_GROUP_SIZE 8

GLuint g_windowW = 1600;
GLuint g_windowH = 1600;

struct NoiseData
{
	GLuint type; //0 = perlin, 1 = worly
	float seed;

	float startFreq;
	GLuint octaves;
};

//--------------------------------------------------------------------------------------------------------------------------------//
//UTILITY FUNCTIONS:

void render_noise_ui(NoiseData* data, bool primary);
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

	//noise shader 2D:
	int noiseShader2D = compile_shader("shaders/noise2D.comp", GL_COMPUTE_SHADER);
	if(noiseShader2D < 0)
	{
		printf("ERROR - COULD NOT COMPILE 2D NOISE SHADER");
		return -1;
	}

	GLuint noiseProgram2D = glCreateProgram();
	glAttachShader(noiseProgram2D, noiseShader2D);
	glLinkProgram(noiseProgram2D);

	GLint linkSuccess;
	glGetProgramiv(noiseProgram2D, GL_LINK_STATUS, &linkSuccess);
	if(!linkSuccess)
	{
		printf("ERROR - COULD NOT LINK 2D NOISE PROGRAM\n");
		return -1;
	}

	glDeleteShader(noiseShader2D);

	//noise shader 3D:
	int noiseShader3D = compile_shader("shaders/noise3D.comp", GL_COMPUTE_SHADER);
	if(noiseShader3D < 0)
	{
		printf("ERROR - COULD NOT COMPILE 3D NOISE SHADER");
		return -1;
	}

	GLuint noiseProgram3D = glCreateProgram();
	glAttachShader(noiseProgram3D, noiseShader3D);
	glLinkProgram(noiseProgram3D);

	glGetProgramiv(noiseProgram3D, GL_LINK_STATUS, &linkSuccess);
	if(!linkSuccess)
	{
		printf("ERROR - COULD NOT LINK 3D NOISE PROGRAM\n");
		return -1;
	}

	glDeleteShader(noiseShader3D);

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

	//generate textures:
	//---------------------------------
	GLuint textureSize = 128;
	GLuint lastTextureSize = textureSize; //for determining if texture needs to be resized

	GLuint noiseTexture2D;
	glGenTextures(1, &noiseTexture2D);
	glBindTexture(GL_TEXTURE_2D, noiseTexture2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	GLuint noiseTexture3D;
	glGenTextures(1, &noiseTexture3D);
	glBindTexture(GL_TEXTURE_3D, noiseTexture3D);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, textureSize, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//generate data buffer:
	//---------------------------------
	GLuint noiseDataBuffer;
	glGenBuffers(1, &noiseDataBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, noiseDataBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(NoiseData) * 8, NULL, GL_DYNAMIC_DRAW);

	//parameter declarations:
	//---------------------------------
	bool is3D = false;

	const char* textureSizeNames[7] = {"16", "32", "64", "128", "256", "512", "1024"};
	int selectedTextureSize = 3;

	const char* channelNames[5] = {"R", "G", "B", "A", "ALL"};
	GLint channelView = 0;
	bool grayscaleChannel = true;
	float slice = 0.0f;

	const char* layeringNames[3] = {"No Layering", "Multiply", "Average"};
	GLuint layeringType = 0;

	NoiseData noiseData[8];
	noiseData[0] = {0, 0.0f, 4.0f, 16};
	noiseData[1] = {0, 1.0f, 4.0f, 16};
	noiseData[2] = {0, 2.0f, 4.0f, 16};
	noiseData[3] = {0, 3.0f, 4.0f, 16};
	noiseData[4] = {0, 4.0f, 4.0f, 16};
	noiseData[5] = {0, 5.0f, 4.0f, 16};
	noiseData[6] = {0, 6.0f, 4.0f, 16};
	noiseData[7] = {0, 7.0f, 4.0f, 16};

	//main loop:
	//---------------------------------
	while(!glfwWindowShouldClose(window))
	{
		//clear
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//send noise data:
		glBindBuffer(GL_UNIFORM_BUFFER, noiseDataBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(NoiseData) * 8, noiseData);

		//dispatch compute shader:
		GLuint numWorkGroups = (GLuint)ceilf((float)textureSize / WORK_GROUP_SIZE);
		if(is3D)
		{
			glUseProgram(noiseProgram3D);

			glUniform1ui(glGetUniformLocation(noiseProgram3D, "layeringType"), layeringType);
			glBindImageTexture(0, noiseTexture3D, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, noiseDataBuffer);

			glDispatchCompute(numWorkGroups, numWorkGroups, numWorkGroups);
		}
		else
		{
			glUseProgram(noiseProgram2D);

			glUniform1ui(glGetUniformLocation(noiseProgram2D, "layeringType"), layeringType);
			glBindImageTexture(0, noiseTexture2D, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, noiseDataBuffer);

			glDispatchCompute(numWorkGroups, numWorkGroups, 1);
		}
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//run final quad shader:
		float screenAspect = (float)g_windowW / g_windowH;

		float scale[2];
		scale[0] = screenAspect > 1.0f ? 1.0f / screenAspect : 1.0f;
		scale[1] = screenAspect < 1.0f ? screenAspect / 1.0f : 1.0f;

		glUseProgram(quadProgram);

		glUniform2fv(glGetUniformLocation(quadProgram, "scale"), 1, scale);
		glUniform1i(glGetUniformLocation(quadProgram, "channel"), channelView);
		glUniform1ui(glGetUniformLocation(quadProgram, "grayscale"), (GLboolean)grayscaleChannel);
		glUniform1ui(glGetUniformLocation(quadProgram, "is3D"), (GLuint)is3D);
		glUniform1f(glGetUniformLocation(quadProgram, "slice"), slice);

		glUniform1i(glGetUniformLocation(quadProgram, "tex2D"), 0);
		glUniform1i(glGetUniformLocation(quadProgram, "tex3D"), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, noiseTexture2D);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, noiseTexture3D);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(0 * sizeof(unsigned int)));

		//render gui:
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Texture Settings");

		ImGui::Combo("Size", &selectedTextureSize, textureSizeNames, 7);
		textureSize = strtoul(textureSizeNames[selectedTextureSize], NULL, 10);
		ImGui::SameLine();
		ImGui::Checkbox("3D", &is3D);

		ImGui::SetNextItemWidth(ImGui::GetWindowSize().x * 0.25f);
		ImGui::Combo("Channel", &channelView, channelNames, 5);
		ImGui::SameLine();
		ImGui::Checkbox("Grayscale", &grayscaleChannel);

		if(is3D)
			ImGui::SliderFloat("Slice", &slice, 0.0f, 1.0f);

		ImGui::Combo("Layering Type", (int*)&layeringType, layeringNames, 3);

		if(ImGui::Button("Export"))
		{
			size_t size = textureSize * textureSize * 4 * sizeof(uint8_t);
			if(is3D)
				size *= textureSize;
			
			void* pixels = malloc(size);
			if(is3D)
			{
				glBindTexture(GL_TEXTURE_3D, noiseTexture3D);
				glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, noiseTexture2D);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			}

			FILE* fptr = fopen("output/out.dat", "wb");
			if(!fptr)
			{
				printf("ERROR - FAILED TO OPEN OUTPUT FILE");
				break;
			}
			
			fwrite(&textureSize, sizeof(GLuint), 1, fptr);
			fwrite(pixels, size, 1, fptr);
			fclose(fptr);
			free(pixels);
		}

		ImGui::End();

		if(channelView < 4)
		{
			render_noise_ui(&noiseData[channelView], true);
			if(layeringType > 0)
				render_noise_ui(&noiseData[channelView + 4], false);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		//resize texture if needed:
		if(textureSize != lastTextureSize)
		{
			glBindTexture(GL_TEXTURE_2D, noiseTexture2D);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_3D, noiseTexture3D);
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, textureSize, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			lastTextureSize = textureSize;
		}

		//swap buffers and poll events:
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//clean up and return:
	glDeleteTextures(1, &noiseTexture2D);
	glDeleteTextures(1, &noiseTexture3D);

	glDeleteVertexArrays(1, &quadVAO);
	
	glDeleteProgram(quadProgram);
	glDeleteProgram(noiseProgram2D);
	glDeleteProgram(noiseProgram3D);
	
	glfwTerminate();

	return 0;
}

//--------------------------------------------------------------------------------------------------------------------------------//

void render_noise_ui(NoiseData* data, bool primary)
{
	if(primary)
		ImGui::Begin("Primary Noise Settings");
	else
		ImGui::Begin("Layered Noise Settings");

	ImGui::PushItemWidth(ImGui::GetWindowSize().x * 0.5f);

	const char* noiseNames[3] = {"Perlin", "Worly", "Worly (Inverted)"};
	ImGui::Combo("Noise Type", (int*)&data->type, noiseNames, 3);
	
	ImGui::InputFloat("Seed", &data->seed, 1.0f, 10.0f);

	const char* freqNames[7] = {"1", "2", "4", "8", "16", "32", "64"};
	int selectedFreq = (int)log2f(data->startFreq);
	ImGui::Combo("Base Frequency", &selectedFreq, freqNames, 7);
	data->startFreq = powf(2.0f, (float)selectedFreq);

	ImGui::SliderInt("Octaves", (int*)&data->octaves, 1, 32);

	ImGui::End();
}

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
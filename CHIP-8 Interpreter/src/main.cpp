
#include "CHIP-8.h"

void GLAPIENTRY errorOccurredGL(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	printf("Message from OpenGL:\nSource: 0x%x\nType: 0x%x\n"
		"Id: 0x%x\nSeverity: 0x%x\n", source, type, id, severity);
	printf("%s\n", message);
}

static unsigned int CompileShader(unsigned int type, const std::string &source) 
{
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) 
	{
		int length;
		char message[1024];
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		glGetShaderInfoLog(id, length, &length, message);
		LOG(message);
	}

	return id;
}

static unsigned int CreateShader(const std::string &vertexSource, const std::string& fragmentSource) 
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);
	glValidateProgram(program);

	glDetachShader(program, vs);
	glDetachShader(program, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

int main() {
	GLFWwindow* window;

	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(640, 320, "CHIP-8", NULL, NULL);

	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
	{
		return - 1;
	}

	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(errorOccurredGL, NULL);

	CHIP8 chip8(window);
	
	float positions[] = {
		-1.0f, -1.0f,
		-1.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f,
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	unsigned int va;

	// Create and bind vertex array
	glGenVertexArrays(1, &va);
	glBindVertexArray(va);

	// Create and bind vertex buffer
	unsigned int vb;
	glGenBuffers(1, &vb);
	glBindBuffer(GL_ARRAY_BUFFER, vb);
	glBufferData(GL_ARRAY_BUFFER, 2 * 4 * sizeof(float), positions, GL_STATIC_DRAW);
	
	// Create vertex attrib pointer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	//create and bind index buffer
	unsigned int ib;
	glGenBuffers(1, &ib);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(float), indices, GL_STATIC_DRAW);

	// Create texture
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 64, 32, 0, GL_RED, GL_UNSIGNED_BYTE, (const void *)chip8.videoBuffer.data());

	// set tex parameteers to default
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glActiveTexture(GL_TEXTURE0);

	const std::string vertexShader = R"glsl(
		#version 330 core

		layout(location = 0) in vec4 position;

		out vec2 v_TexCoord;

		void main() {
		   gl_Position = position;
		   v_TexCoord = vec2(position.x * 0.5f + 0.5f, 1 - (position.y * 0.5f + 0.5f));
		}
	)glsl";

	const std::string fragmentShader = R"glsl(
		#version 330 core

		layout(location = 0) out vec4 color;

		in vec2 v_TexCoord;

		uniform sampler2D u_Texture;

		void main()
		{
			vec4 texColor = texture(u_Texture, v_TexCoord);
			color = vec4(vec3(texColor.r), 1);
		}
	)glsl";

	unsigned int shader = CreateShader(vertexShader, fragmentShader);
	glUseProgram(shader);
	glUniform1i(glGetUniformLocation(shader, "u_Texture"), 0);

	chip8.loadFont();
	chip8.loadProgram("./res/chip8picture.ch8");

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		if (chip8.nextInstruction();chip8.Opcode != 0x0000)
		{
			chip8.executeInstruction();
		}

		glfwSwapBuffers(window);
	}
	glDeleteProgram(shader);
	glfwTerminate();
}
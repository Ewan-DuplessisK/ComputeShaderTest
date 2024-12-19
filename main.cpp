#include<iostream>

#include"glad/glad.h"
#include"GLFW/glfw3.h"
#include <ctime>



const unsigned int SCREEN_WIDTH = 1024;
const unsigned int SCREEN_HEIGHT = 1024;

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

bool vSync = true;



GLfloat vertices[] =
{
	-1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
	 1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
};

GLuint indices[] =
{
	0, 2, 1,
	0, 3, 2
};


const char* screenVertexShaderSource = R"(#version 460 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvs;
out vec2 UVs;
void main()
{
	gl_Position = vec4(pos.x, pos.y, pos.z, 1.0);
	UVs = uvs;
})";
const char* screenFragmentShaderSource = R"(#version 460 core
out vec4 FragColor;
uniform sampler2D screen;
in vec2 UVs;
void main()
{
	FragColor = texture(screen, UVs);
})";
const char* screenComputeShaderSource = R"(#version 430

layout(local_size_x = 16, local_size_y = 16) in;
//layout(rgba32f, binding = 1) uniform image2D img_input;
layout(rgba32f, binding = 0) uniform image2D screen;
uniform float inRand;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main(){
    // Base pixel colour for image
    vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);
    // Get index in global work group i.e x,y position
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    ivec2 dims = imageSize(/*img_input*/screen); // fetch image dimensions
    float x = -(float(pixel_coords.x*2-dims.x)/dims.x);
    float y = -(float(pixel_coords.y*2-dims.y)/dims.y);

    float rand = rand(pixel_coords*inRand);
    float heat = 0.f;
    for(int i=-1;i<2;i++){
        for(int j=-1;j<2;j++){
            if(pixel_coords.x+(i/dims.x)>=0 && pixel_coords.x+(i/dims.x)<dims.x && pixel_coords.y+(j/dims.y)>=0 && pixel_coords.y+(j/dims.y)<dims.y){
                vec4 nPixel = imageLoad(/*img_input*/screen,ivec2(x+(i/dims.x),y+(j/dims.y)));
                heat+= nPixel.x + nPixel.y;
            }
        }
    }
    heat/=9.f;
    if(heat<.8f){
        if(rand<.1f)heat+=-.01f;
        else if(rand<.6f)heat+=.01f;
        else if(rand<=1.f)heat+=.02f; //check in case rand goes over 1
    }
    else if(heat<1.3f){
        if(rand<.2f)heat+=-.01f;
        else if(rand<.8f)heat+=.01f;
        else if(rand<=1.0f)heat+=.02f;
    }
    else if(heat<1.8f){
        if(rand<.5f)heat+=-.01f;
        else if(rand<=1.f)heat+=.02f;
    }
    else{
        if(rand<.5f)heat-=.01;
        else if (rand<=1.f)heat-=.02f;
    }
    
    
    if(heat<1.f)pixel=vec4(heat,0.f,0.f,1.f);
    else pixel=vec4(1.f,heat-1.f,0.f,1.f);

    imageStore(screen, pixel_coords, pixel);
})";


int main()
{
	srand(time(nullptr));
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VERSION);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Compute Shaders", NULL, NULL);
	if (!window)
	{
		std::cout << "Failed to create the GLFW window\n";
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(vSync);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
	}
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);


	GLuint VAO, VBO, EBO;
	glCreateVertexArrays(1, &VAO);
	glCreateBuffers(1, &VBO);
	glCreateBuffers(1, &EBO);

	glNamedBufferData(VBO, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glNamedBufferData(EBO, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(VAO, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

	glEnableVertexArrayAttrib(VAO, 1);
	glVertexArrayAttribBinding(VAO, 1, 0);
	glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));

	glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(GLfloat));
	glVertexArrayElementBuffer(VAO, EBO);


	GLuint screenTex;
	glCreateTextures(GL_TEXTURE_2D, 1, &screenTex);
	glTextureParameteri(screenTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(screenTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(screenTex, 1, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindImageTexture(0, screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	GLuint inputTex;
	glCreateTextures(GL_TEXTURE_2D, 1, &inputTex);
	glTextureParameteri(inputTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(inputTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(inputTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(inputTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(inputTex, 1, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT);
	glBindImageTexture(1, inputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	GLuint screenVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(screenVertexShader, 1, &screenVertexShaderSource, NULL);
	glCompileShader(screenVertexShader);
	GLuint screenFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(screenFragmentShader, 1, &screenFragmentShaderSource, NULL);
	glCompileShader(screenFragmentShader);

	GLuint screenShaderProgram = glCreateProgram();
	glAttachShader(screenShaderProgram, screenVertexShader);
	glAttachShader(screenShaderProgram, screenFragmentShader);
	glLinkProgram(screenShaderProgram);

	glDeleteShader(screenVertexShader);
	glDeleteShader(screenFragmentShader);


	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(computeShader, 1, &screenComputeShaderSource, NULL);
	glCompileShader(computeShader);

	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader);
	glLinkProgram(computeProgram);


	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	std::cout << "Max work groups per compute shader" << 
		" x:" << work_grp_cnt[0] <<
		" y:" << work_grp_cnt[1] <<
		" z:" << work_grp_cnt[2] << "\n";

	int work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	std::cout << "Max work group sizes" <<
		" x:" << work_grp_size[0] <<
		" y:" << work_grp_size[1] <<
		" z:" << work_grp_size[2] << "\n";

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	std::cout << "Max invocations count per work group: " << work_grp_inv << "\n";


	while (!glfwWindowShouldClose(window))
	{
		glUseProgram(computeProgram);
		glDispatchCompute(ceil(SCREEN_WIDTH / 16), ceil(SCREEN_HEIGHT / 16), 1);
		float r = float(rand())/float(RAND_MAX);
		glBindTextureUnit(1, inputTex);
		glUniform1i(glGetUniformLocation(computeProgram, "img_input"), 1);
		glUniform1f(glGetUniformLocation(computeProgram,"inRand"),r);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glUseProgram(screenShaderProgram);
		glBindTextureUnit(0, screenTex);
		glUniform1i(glGetUniformLocation(screenShaderProgram, "screen"), 0);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(indices[0]), GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	glfwDestroyWindow(window);
	glfwTerminate();
}
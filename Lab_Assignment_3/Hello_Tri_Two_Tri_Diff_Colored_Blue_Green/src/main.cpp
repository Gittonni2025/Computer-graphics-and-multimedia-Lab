#include "glad.h"
#include "glfw3.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// Window settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Shaders
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);\n" // Cyan Color
    "}\n\0";

int main()
{
    // Initialize and configure GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "0432410005101024", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Build and compile shader program
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 5-pointed star geometry made of 10 individual triangles connected to the center (0,0)
    float starVertices[] = {
        // Triangle 1
        0.0f,  0.0f,  0.0f,    0.0f,   0.8f,   0.0f,   0.224f,  0.309f, 0.0f,
        // Triangle 2
        0.0f,  0.0f,  0.0f,    0.224f, 0.309f, 0.0f,   0.761f,  0.247f, 0.0f,
        // Triangle 3
        0.0f,  0.0f,  0.0f,    0.761f, 0.247f, 0.0f,   0.363f, -0.118f, 0.0f,
        // Triangle 4
        0.0f,  0.0f,  0.0f,    0.363f,-0.118f, 0.0f,   0.470f, -0.647f, 0.0f,
        // Triangle 5
        0.0f,  0.0f,  0.0f,    0.470f,-0.647f, 0.0f,   0.0f,   -0.309f, 0.0f,
        // Triangle 6
        0.0f,  0.0f,  0.0f,    0.0f,  -0.309f, 0.0f,  -0.470f, -0.647f, 0.0f,
        // Triangle 7
        0.0f,  0.0f,  0.0f,   -0.470f,-0.647f, 0.0f,  -0.363f, -0.118f, 0.0f,
        // Triangle 8
        0.0f,  0.0f,  0.0f,   -0.363f,-0.118f, 0.0f,  -0.761f,  0.247f, 0.0f,
        // Triangle 9
        0.0f,  0.0f,  0.0f,   -0.761f, 0.247f, 0.0f,  -0.224f,  0.309f, 0.0f,
        // Triangle 10
        0.0f,  0.0f,  0.0f,   -0.224f, 0.309f, 0.0f,   0.0f,    0.8f,   0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(starVertices), starVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Main Render Loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Yellow Background Color (Red=1.0, Green=1.0, Blue=0.0)
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render Star
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 30); // 10 triangles * 3 vertices = 30 total vertices

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
#include "glad.h"
#include "glfw3.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Vertex Shader
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";

// Cyan Fragment Shader for Square (R=0, G=1, B=1)
const char *fragmentShaderCyanSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);\n"
    "}\n\0";

// Magenta Fragment Shader for Triangle (R=1, G=0, B=1)
const char *fragmentShaderMagentaSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);\n"
    "}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Window Creation with Full Name Title
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Nusrat Binte Alam Tonni", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Build and compile Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Cyan Shader Program (Square)
    unsigned int fragmentShaderCyan = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderCyan, 1, &fragmentShaderCyanSource, NULL);
    glCompileShader(fragmentShaderCyan);

    unsigned int shaderProgramCyan = glCreateProgram();
    glAttachShader(shaderProgramCyan, vertexShader);
    glAttachShader(shaderProgramCyan, fragmentShaderCyan);
    glLinkProgram(shaderProgramCyan);

    // Magenta Shader Program (Triangle)
    unsigned int fragmentShaderMagenta = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderMagenta, 1, &fragmentShaderMagentaSource, NULL);
    glCompileShader(fragmentShaderMagenta);

    unsigned int shaderProgramMagenta = glCreateProgram();
    glAttachShader(shaderProgramMagenta, vertexShader);
    glAttachShader(shaderProgramMagenta, fragmentShaderMagenta);
    glLinkProgram(shaderProgramMagenta);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShaderCyan);
    glDeleteShader(fragmentShaderMagenta);

    // 1. Cyan Square Vertices (2 Triangles = 6 Vertices)
    float cyanSquare[] = {
        -0.4f, -0.4f, 0.0f,  // Bottom-Left
         0.4f, -0.4f, 0.0f,  // Bottom-Right
        -0.4f,  0.4f, 0.0f,  // Top-Left

         0.4f, -0.4f, 0.0f,  // Bottom-Right
         0.4f,  0.4f, 0.0f,  // Top-Right
        -0.4f,  0.4f, 0.0f   // Top-Left
    };

    // 2. Magenta Triangle Vertices (Shares Top-Left and Top-Right corners of the square)
    float magentaTriangle[] = {
        -0.4f,  0.4f, 0.0f,  // Top-Left Corner of Square
         0.4f,  0.4f, 0.0f,  // Top-Right Corner of Square
         0.0f,  0.8f, 0.0f   // Top Point Above Square
    };

    unsigned int VBOs[2], VAOs[2];
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);

    // Setup Cyan Square Buffer
    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyanSquare), cyanSquare, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Setup Magenta Triangle Buffer
    glBindVertexArray(VAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(magentaTriangle), magentaTriangle, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Render Loop
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // White Background Color
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw Cyan Square
        glUseProgram(shaderProgramCyan);
        glBindVertexArray(VAOs[0]);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw Magenta Triangle on top
        glUseProgram(shaderProgramMagenta);
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);
    glDeleteProgram(shaderProgramCyan);
    glDeleteProgram(shaderProgramMagenta);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

   
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

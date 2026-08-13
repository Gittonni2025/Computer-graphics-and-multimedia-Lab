#include "glad.h"
#include "glfw3.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//Vertex Shader
// *vertexShaderSource is a pointer to a constant character string,  contains the source code of the vertex shader.
const char *vertexShaderSource = "#version 330 core\n" //#version 330 core is a directive to specify the version of OpenGL Shading Language
    "layout (location = 0) in vec3 aPos;\n" //When you set up your vertex data in OpenGL, you're sending a series of values, like positions or colors. By specifying the layout and location before sending to GPU, you're essentially telling OpenGL which parts of the data correspond to which attribute in the shader.
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n" //4th coordinate, 1.0 = position point, 0.0 = direction vector
    "}\0"; //explicit end marker, terminated by null

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
           "FragColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);\n" /*vec4(1.0f, 0.0f, 0.0f, 1.0f): This creates a 4D vector with four components, each representing one of the color channels (red, green, blue, and alpha):
            1.0f: Red component (full red).
            0.0f: Green component (no green).
            0.0f: Blue component (no blue).
            1.0f: Alpha component (fully opaque).*/
    "}\n\0";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Nusrat Binte Alam Tonni", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER); //glCreateShader(GL_VERTEX_SHADER) creates a new vertex shader and returns a non-negative integer (the shader handle). OpenGL shader handles integer type identifiers for shaders

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // 1 for single string, NULL is the string is terminated by null.

    glCompileShader(vertexShader); //check for errors in shader language
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success); //get compile status yes or no
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog); //512 length of errorlog
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }


    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //creates new fragment type shader

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); //binds with upper code string

    glCompileShader(fragmentShader);     // check for shader compile errors

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }


    // link shaders
    unsigned int shaderProgram = glCreateProgram(); /*creates an empty program object in OpenGL, which will later be used to attach shaders (like vertex and fragment shaders) and link them together into a final program that can be executed by the GPU.*/

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram); /* link them together into a complete and executable shader program. Once the shaders are linked, the program can be used to render objects in OpenGL.*/


        // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    //No need now 
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
 float vertices[] = {
    // First Triangle
    -0.5f, -0.5f, 0.0f,  // Bottom-left
     0.6f, -0.5f, 0.0f,  // Bottom-right
     -0.7f,  0.5f, 0.0f,  // Top-right  
};
    unsigned int VBO, VAO; /*Here, we are declaring two variables:
    VBO (Vertex Buffer Object): This will store the actual vertex data (like positions, colors, etc.).
    VAO (Vertex Array Object): This is used to store the configuration about how the vertex data is laid out.
    */


    glGenVertexArrays(1, &VAO);/*This creates a Vertex Array Object (VAO), and we store its ID in VAO. A VAO will keep track of all the vertex-related states (such as how the vertex data is stored and how it's accessed).*/

    glGenBuffers(1, &VBO); /*creates a Vertex Buffer Object (VBO), and we store its ID in VBO. A VBO is where the actual vertex data (e.g., the positions of the triangle's corners) will go.*/


    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO); //Activate VAO, now below this, all VBO will be binded to this VAO. Open the folder for storing VBO

    glBindBuffer(GL_ARRAY_BUFFER, VBO); //type of data: GL_ARRAY_BUFFER specifically refers to buffers that store vertex data, A VBO is a memory block (managed by OpenGL) that holds your vertex data (e.g., positions, colors).

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); 
    /*the buffer you're working with will store vertex data,
    size of the data that you want to store in the buffer.
    the pointer to the array vertices,
    GL_STATIC_DRAW indicates that the data in the buffer will not change frequently */

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //how to interpret the data you have just uploaded
    /*This is the location of the vertex attribute,
    each vertex consists of three components: the x, y, and z,
     type of the data.
     normalize the values between -1.0 and 1.0,
     stride (the space between the beginning of two consecutive attributes) for each vertex.
     The data starts at the very beginning of the buffer ((void*)0). NO OFFSET

    */
    glEnableVertexAttribArray(0); // enable the vertex attribute array(VAO) for a specific attribute location.

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); //clean the board, all uploaded done

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); //inactivate VAO, all copied to GPU


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        // Inside render loop
glClearColor(1.0f, 0.64f, 0.0f, 1.0f); // Yellow background
glClear(GL_COLOR_BUFFER_BIT); 
        

        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // Draw 6 vertices instead of 3//0 starting index, 3 no. of vertices
        // glBindVertexArray(0); // no need to unbind it every time 
 
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO); //delete 1 VAO
    glDeleteBuffers(1, &VBO); //delete 1 VBO
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
/*You can check whether your system is using integrated graphics or software rendering by:

Windows:
Open Run (Win + R) → Type dxdiag → Check Display tab.
Or, use GPU-Z (free tool) to see OpenGL support.
Linux/macOS:
Run glxinfo | grep "OpenGL renderer" in the terminal.*/

/*Student Work
1. Upside down green triangle in black background
2. Right Angle Magenta triangle in white background*/
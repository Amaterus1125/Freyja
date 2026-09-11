#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>

// --- Shaders ---
// Vertex shader now takes TWO inputs: position (location 0) and color (location 1).
// It passes the color through to the fragment shader via an "out" variable.
const char* vertexShaderSrc = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor; // sent to the fragment shader

void main() {
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

// Fragment shader receives vertexColor, but interpolated!
// OpenGL automatically blends the 3 vertex colors based on how close
// each pixel is to each corner of the triangle -- that's the gradient.
const char* fragmentShaderSrc = R"(
#version 460 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::fprintf(stderr, "Shader compile error: %s\n", infoLog);
    }
    return shader;
}

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Engine", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to init GLAD\n");
        return -1;
    }

    std::printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    // --- Triangle vertex data ---
    // Each vertex now has 6 floats: x, y, z, r, g, b
    float vertices[] = {
        // positions          // colors
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f, // bottom left  -> red
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, // bottom right -> green
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f  // top          -> blue
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Each vertex is now 6 floats (24 bytes) apart -- that's the "stride".
    // Attribute 0: position -- 3 floats, starting at offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1: color -- 3 floats, starting right after position (offset = 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Shader program ---
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::fprintf(stderr, "Shader link error: %s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- Main loop ---
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

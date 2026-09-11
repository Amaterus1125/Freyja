#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cstring>


static const char* shaderCodeVertex = R"(
#version 460 core

layout (std140, binding = 0) uniform PerFrameData {
    uniform mat4 MVP;
    uniform int isWireframe;
};

layout (location = 0) out vec3 color;

const vec3 pos[8] = vec3[8](
    vec3(-1.0,-1.0, 1.0), vec3( 1.0,-1.0, 1.0), vec3( 1.0, 1.0, 1.0), vec3(-1.0, 1.0, 1.0),
    vec3(-1.0,-1.0,-1.0), vec3( 1.0,-1.0,-1.0), vec3( 1.0, 1.0,-1.0), vec3(-1.0, 1.0,-1.0)
);

const vec3 col[8] = vec3[8](
    vec3(1.0,0.0,0.0), vec3(0.0,1.0,0.0), vec3(0.0,0.0,1.0), vec3(1.0,1.0,0.0),
    vec3(1.0,1.0,0.0), vec3(0.0,0.0,1.0), vec3(0.0,1.0,0.0), vec3(1.0,0.0,0.0)
);

const int indices[36] = int[36](
    0,1,2, 2,3,0,   // front
    1,5,6, 6,2,1,   // right
    7,6,5, 5,4,7,   // back
    4,0,3, 3,7,4,   // left
    4,5,1, 1,0,4,   // bottom
    3,2,6, 6,7,3    // top
);

void main() {
    int idx = indices[gl_VertexID];
    gl_Position = MVP * vec4(pos[idx], 1.0);
    color = isWireframe > 0 ? vec3(0.0) : col[idx];
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core

layout (location = 0) in vec3 color;
layout (location = 0) out vec4 out_FragColor;

void main() {
    out_FragColor = vec4(color, 1.0);
}
)";


struct PerFrameData {
    glm::mat4 mvp;
    int isWireframe;
};

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return shader;
}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Cube", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glEnable(GL_DEPTH_TEST);

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, shaderCodeVertex);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, shaderCodeFragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    const GLsizeiptr kBufferSize = sizeof(PerFrameData);
    GLuint perFrameDataBuf;
    glCreateBuffers(1, &perFrameDataBuf);
    glNamedBufferStorage(perFrameDataBuf, kBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuf, 0, kBufferSize);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 p = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 m = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.5f)),
            (float)glfwGetTime(),
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        glUseProgram(program);

        PerFrameData perFrameData{ .mvp = p * m, .isWireframe = 0 };
        glNamedBufferSubData(perFrameDataBuf, 0, kBufferSize, &perFrameData);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        perFrameData.isWireframe = 1;
        glNamedBufferSubData(perFrameDataBuf, 0, kBufferSize, &perFrameData);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &perFrameDataBuf);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}

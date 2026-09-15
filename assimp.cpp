#include <vector>
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using glm::vec3;
using glm::mat4;

static const char* kVertexShaderSrc = R"(
#version 450 core
layout (location = 0) in vec3 aPos;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

static const char* kFragmentShaderSrc = R"(
#version 450 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        printf("Shader compile error: %s\n", log);
        exit(255);
    }
    return shader;
}

static GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

int main() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int windowWidth = 1024;
    const int windowHeight = 768;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "engine", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
    std::vector<vec3> positions;
    const aiMesh* mesh = scene->mMesh[0];
    vec3 boundsMin( 1e30f);
    vec3 boundsMax(-1e30f);

    for (unsigned int i = 0; i != mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        const unsigned int idx[3] = {face.mIndices[0], face.mIndices[1], face.mIndices[2]};
        for (int j = 0; j != 3; j++) {
            const aiVector3D v = mesh->mVertices[idx[j]];
            vec3 p(v.x, v.z, v.y);
            positions.push_back(p);
            boundsMin = glm::min(boundsMin, p);
            boundsMax = glm::max(boundsMax, p);
        }
    }
    aiReleaseImport(scene);

    GLuint VAO;
    glCreateVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    GLuint meshData;
    glCreateBuffers(1, &meshData);
    glNamedBufferStorage(meshData, sizeof(vec3) * positions.size(), positions.data(), 0);
    glVertexArrayVertexBuffer(VAO, 0, meshData, 0, sizeof(vec3));
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);
    const int numVertices = static_cast<int>(positions.size());

    GLuint program = createProgram(kVertexShaderSrc, kFragmentShaderSrc);
    GLint mvpLoc = glGetUniformLocation(program, "MVP");
    GLint colorLoc = glGetUniformLocation(program, "uColor");

    vec3 center = 0.5f * (boundsMin + boundsMax);
    float radius = glm::length(boundsMax - boundsMin) * 0.5f;
    if (radius < 0.0001f) radius = 1.0f;

    mat4 model = mat4(1.0f);
    mat4 view = glm::lookAt(center + vec3(0.0f, -radius * 2.5f, radius * 0.5f),
                             center,
                             vec3(0.0f, 0.0f, 1.0f));
    mat4 projection = glm::perspective(glm::radians(60.0f),
                                        (float)fbWidth / (float)fbHeight,
                                        0.01f, radius * 100.0f);
    mat4 mvp = projection * view * model;

    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glBindVertexArray(VAO);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniform4f(colorLoc, 0.8f, 0.7f, 0.2f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);

        glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, numVertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &meshData);
    glDeleteVertexArrays(1, &VAO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

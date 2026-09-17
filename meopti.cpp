#include <vector>
#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>

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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int windowWidth = 1024;
    const int windowHeight = 768;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "engine", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
    const aiMesh* mesh = scene->mMeshes[0];

    std::vector<vec3> positions;
    std::vector<unsigned int> indices;
    positions.reserve(mesh->mNumVertices);
    for (unsigned i = 0; i != mesh->mNumVertices; i++) {
        const aiVector3D v = mesh->mVertices[i];
        positions.push_back(vec3(v.x, v.z, v.y));
    }
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned i = 0; i != mesh->mNumFaces; i++) {
        for (unsigned j = 0; j != 3; j++) {
            indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
    aiReleaseImport(scene);

    vec3 boundsMin(1e30f), boundsMax(-1e30f);
    for (const vec3& p : positions) {
        boundsMin = glm::min(boundsMin, p);
        boundsMax = glm::max(boundsMax, p);
    }
    std::vector<unsigned int> remap(indices.size());
    const size_t vertexCount = meshopt_generateVertexRemap(
        remap.data(), indices.data(), indices.size(),
        positions.data(), positions.size(), sizeof(vec3));

    std::vector<unsigned int> remappedIndices(indices.size());
    std::vector<vec3> remappedVertices(vertexCount);
    meshopt_remapIndexBuffer(remappedIndices.data(), indices.data(), indices.size(), remap.data());
    meshopt_remapVertexBuffer(remappedVertices.data(), positions.data(), positions.size(), sizeof(vec3), remap.data());

    meshopt_optimizeVertexCache(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(), vertexCount);
    meshopt_optimizeOverdraw(remappedIndices.data(), remappedIndices.data(), remappedIndices.size(),
                              glm::value_ptr(remappedVertices[0]), vertexCount, sizeof(vec3), 1.05f);
    meshopt_optimizeVertexFetch(remappedVertices.data(), remappedIndices.data(), remappedIndices.size(),
                                 remappedVertices.data(), vertexCount, sizeof(vec3));

    const float threshold = 0.2f;
    const size_t target_index_count = size_t(remappedIndices.size() * threshold);
    const float target_error = 1e-2f;
    std::vector<unsigned int> indicesLod(remappedIndices.size());
    indicesLod.resize(meshopt_simplify(
        indicesLod.data(), remappedIndices.data(), remappedIndices.size(),
        &remappedVertices[0].x, vertexCount, sizeof(vec3),
        target_index_count, target_error));
    indices = remappedIndices;
    positions = remappedVertices;
    const size_t sizeIndices = sizeof(unsigned int) * indices.size();
    const size_t sizeIndicesLod = sizeof(unsigned int) * indicesLod.size();
    const size_t sizeVertices = sizeof(vec3) * positions.size();

    GLuint VAO;
    glCreateVertexArrays(1, &VAO);

    GLuint meshData;
    glCreateBuffers(1, &meshData);
    glNamedBufferStorage(meshData, sizeIndices + sizeIndicesLod + sizeVertices, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glNamedBufferSubData(meshData, 0, sizeIndices, indices.data());
    glNamedBufferSubData(meshData, sizeIndices, sizeIndicesLod, indicesLod.data());
    glNamedBufferSubData(meshData, sizeIndices + sizeIndicesLod, sizeVertices, positions.data());

    glVertexArrayElementBuffer(VAO, meshData);
    glVertexArrayVertexBuffer(VAO, 0, meshData, sizeIndices + sizeIndicesLod, sizeof(vec3));
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0);

    GLuint program = createProgram(kVertexShaderSrc, kFragmentShaderSrc);
    GLint mvpLoc = glGetUniformLocation(program, "MVP");
    GLint colorLoc = glGetUniformLocation(program, "uColor");

    vec3 center = 0.5f * (boundsMin + boundsMax);
    float radius = glm::length(boundsMax - boundsMin) * 0.5f;
    if (radius < 0.0001f) radius = 1.0f;

    mat4 view = glm::lookAt(center + vec3(0.0f, -radius * 2.5f, radius * 0.5f),
                             center, vec3(0.0f, 0.0f, 1.0f));
    mat4 projection = glm::perspective(glm::radians(60.0f),
                                        (float)fbWidth / (float)fbHeight,
                                        0.01f, radius * 100.0f);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(program);
    glBindVertexArray(VAO);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float angle = (float)glfwGetTime();
        mat4 model = glm::rotate(mat4(1.0f), angle, vec3(0.0f, 0.0f, 1.0f));
        mat4 mvp = projection * view * model;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4f(colorLoc, 0.8f, 0.7f, 0.2f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
        glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, (GLsizei)indicesLod.size(), GL_UNSIGNED_INT, (void*)sizeIndices);
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

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace glm;


GLuint loadCubemap(const char* hdrFile);
extern const char* kVertexShader;
extern const char* kFragmentShader;
const std::string kData = "/home/arc/dev/engine/data/";
struct PerFrameData { mat4 model; mat4 mvp; vec4 cameraPos; };

static const char* kCubeVS = R"(
#version 460 core
layout(std140, binding = 0) uniform PerFrameData { mat4 model; mat4 MVP; vec4 cameraPos; };
layout(location = 0) out vec3 dir;
const vec3 pos[8] = vec3[8](
    vec3(-1,-1, 1), vec3( 1,-1, 1), vec3( 1, 1, 1), vec3(-1, 1, 1),
    vec3(-1,-1,-1), vec3( 1,-1,-1), vec3( 1, 1,-1), vec3(-1, 1,-1));
const int indices[36] = int[36](
    0,1,2, 2,3,0,  1,5,6, 6,2,1,  7,6,5, 5,4,7,
    4,0,3, 3,7,4,  4,5,1, 1,0,4,  3,2,6, 6,7,3);
void main() {
    int idx = indices[gl_VertexID];
    gl_Position = MVP * vec4(100.0 * pos[idx], 1.0);
    dir = pos[idx];})";

static const char* kCubeFS = R"(
#version 460 core
layout(location = 0) in vec3 dir;
layout(location = 0) out vec4 out_FragColor;
layout(binding = 1) uniform samplerCube texture1;
void main() { out_FragColor = texture(texture1, dir); })";

static GLuint makeProgram(const char* vs, const char* fs) {
    auto compile = [](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[4096]; glGetShaderInfoLog(s, 4096, nullptr, log); std::fprintf(stderr, "shader error:\n%s\n", log); }
        return s;};
    GLuint p = glCreateProgram();
    glAttachShader(p, compile(GL_VERTEX_SHADER, vs));
    glAttachShader(p, compile(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[4096]; glGetProgramInfoLog(p, 4096, nullptr, log); std::fprintf(stderr, "link error:\n%s\n", log); }
    return p;
}
int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "reflecting duck", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;
    glfwSwapInterval(1);
    std::filesystem::create_directories("data/out");   // screen.hdr goes here
    GLuint cubemap = loadCubemap("");
    if (!cubemap) { std::fprintf(stderr, "cubemap load failed\n"); return 1; }
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    const aiScene* scene = aiImportFile((kData + "rubber_duck/scene.gltf").c_str(), aiProcess_Triangulate);
    if (!scene || !scene->HasMeshes()) { std::fprintf(stderr, "duck load failed\n"); return 1; }
    const aiMesh* mesh = scene->mMeshes[0];
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    for (unsigned i = 0; i < mesh->mNumVertices; i++) {
        const aiVector3D v = mesh->mVertices[i];
        const aiVector3D n = mesh->mNormals[i];
        const aiVector3D t = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f);
        vertices.insert(vertices.end(), {v.x, v.y, v.z, t.x, t.y, n.x, n.y, n.z});}
    for (unsigned i = 0; i < mesh->mNumFaces; i++)
        for (unsigned j = 0; j < 3; j++) indices.push_back(mesh->mFaces[i].mIndices[j]);
    aiReleaseImport(scene);

    GLuint vbo, ibo, vao;
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, vertices.size() * sizeof(float), vertices.data(), 0);
    glCreateBuffers(1, &ibo);
    glNamedBufferStorage(ibo, indices.size() * sizeof(uint32_t), indices.data(), 0);
    glCreateVertexArrays(1, &vao);
    glVertexArrayElementBuffer(vao, ibo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vbo);

    stbi_set_flip_vertically_on_load(true);
    int tw, th, tc;
    unsigned char* px = stbi_load((kData + "rubber_duck/DuckCM.png").c_str(), &tw, &th, &tc, 4);
    if (!px) { std::fprintf(stderr, "duck texture load failed\n"); return 1; }
    GLuint duckTex;
    glCreateTextures(GL_TEXTURE_2D, 1, &duckTex);
    glTextureParameteri(duckTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(duckTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(duckTex, 1 + int(std::floor(std::log2(float(tw > th ? tw : th)))), GL_RGBA8, tw, th);
    glTextureSubImage2D(duckTex, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glGenerateTextureMipmap(duckTex);
    stbi_image_free(px);
    glBindTextureUnit(0, duckTex);
    glBindTextureUnit(1, cubemap);
    GLuint progDuck = makeProgram(kVertexShader, kFragmentShader);
    GLuint progCube = makeProgram(kCubeVS, kCubeFS);
    GLuint ubo;
    glCreateBuffers(1, &ubo);
    glNamedBufferStorage(ubo, sizeof(PerFrameData), nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);
    while (!glfwWindowShouldClose(window)) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const mat4 p  = perspective(radians(45.0f), w / float(h), 0.1f, 1000.0f);
        const mat4 m1 = rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0));
        const mat4 m2 = rotate(mat4(1.0f), (float)glfwGetTime(), vec3(0, 1, 0));
        const mat4 v  = translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f));
        const mat4 model = v * m2 * m1;
        PerFrameData duck{model, p * model, vec4(0, 0, 0, 1)};
        glNamedBufferSubData(ubo, 0, sizeof(duck), &duck);
        glUseProgram(progDuck);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
        PerFrameData sky{mat4(1.0f), p, vec4(0, 0, 0, 1)};
        glNamedBufferSubData(ubo, 0, sizeof(sky), &sky);
        glUseProgram(progCube);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

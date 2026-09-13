#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <easy/profiler.h>
#include <cstdio>
#include <chrono>
#include <thread>
int main(){
    EASY_MAIN_THREAD;
    EASY_PROFILER_ENABLE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Profile Easy", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    EASY_BLOCK("Create Resources");
    const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);
    const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint perFrameDataBuffer;
    glCreateBuffers(1,&perFrameDataBuffer);
    EASY_END_BLOCK;
    {
        EASY_BLOCK("Set state");
        glClearColor(1.0f,1.0f,1.0f,1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f,-1.0f);
    }
    while (!glfwWindowShouldClose(window)){
        EASY_BLOCK("MainLoop");
        {
            EASY_BLOCK("Part1");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        {
            EASY_BLOCK("PART2b");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        {
            EASY_BLOCK("glfwPollEvents()");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            glfwPollEvents();}}
    profiler::dumpBlocksToFile("profiler_dump.prof");
    glDeleteShader(shaderVertex);
    glDeleteShader(shaderFragment);
    glDeleteBuffers(1, &perFrameDataBuffer);
 
    glfwTerminate();
    return 0;
}







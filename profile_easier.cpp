#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <easy/profiler.h>
int main(){
    EASY_MAIN_THREAD;
    EASY_PROFILER_ENABLE;
    EASY_BLOCK("Create Resources");
    const GLuint shaderVertex = glCreateShader(GL_VERTEX_SHADER);
    const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint perFrameDataBuffer;
    glCreateBuffers(1,&perFrameDataBuffer);
    EASY_END_BLOCK;
    {
        Easy_BLOCK("Set state");
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








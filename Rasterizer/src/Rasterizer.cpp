
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"

#include <iostream>
#include <vector>

#include "Shader.h"

unsigned BuildVAOfromData(const std::vector<float>& vertexes, 
                                const std::vector<float>& colours, 
                                const std::vector<unsigned>& indexes)
{
    //ids for buffer objects
    unsigned vbo[2], vao, ibo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //generate vertex buffers and assign vertexes to first
    glGenBuffers(2, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, vertexes.size() * sizeof(float), vertexes.data(), GL_STATIC_DRAW);

    //shader layout location for vertexes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    //assign colours to second
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, colours.size() * sizeof(float), colours.data(), GL_STATIC_DRAW);

    //shader layout location for colours
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(1);

    //generate index buffer and assign indexes to it
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned), indexes.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    return vao;
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(500, 500, "Rasterizer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    //Shader basicShader("res/shaders/BasicShader.txt");
    //basicShader.Bind();

    //glViewport(0,0,640,480);


    const float pixelrgbs[] {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f
    };

    std::vector<float> pixlrgb;
    pixlrgb.reserve(500 * 500);

    for (int i{0}; i < (500*500); i++){
        int temp { (i % 5) * 3};
        pixlrgb.emplace_back(pixelrgbs[temp]);
        pixlrgb.emplace_back(pixelrgbs[temp + 1]);
        pixlrgb.emplace_back(pixelrgbs[temp + 2]);
    }
    
    int width,height;
    glfwGetWindowSize(window, &width, &height);
    std::cout << "Window width: " << width << ". Window height: " << height << '\n';

    float currentwidth = -1;
    float currentheight = -1;

    int rasterpos[4];
    glRasterPos4f(-1.f,-1.f,0.f,1.f);
    glGetIntegerv(GL_CURRENT_RASTER_POSITION, rasterpos);

    std::cout << "Raster Pos: " << rasterpos[0] << ", " << rasterpos[1] << ", " << rasterpos[2] << ", " << rasterpos[3] << '\n';
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        //input
        glfwPollEvents();

        //render
        glDrawPixels(500, 500, GL_RGB, GL_FLOAT, pixlrgb.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
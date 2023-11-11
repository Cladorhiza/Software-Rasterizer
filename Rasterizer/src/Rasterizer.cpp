
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

#include <iostream>
#include <vector>

const int WIDTH = 640;
const int HEIGHT = 480;
constexpr int FRAMEBUFFER_SIZE = WIDTH * HEIGHT;

const glm::vec4 black{0.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 red{1.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 green{0.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 blue{0.0f, 0.0f, 1.0f, 1.0f};
const glm::vec4 yellow{1.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 burgundy{0.5f, 0.0f, 0.125f, 1.0f};

struct Triangle{

    Triangle(){
    }

    Triangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec4& colour)
        : v1(v1), v2(v2), v3(v3), colour(colour)
    {

    }

    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 v3;
    glm::vec4 colour;
};

struct Shader{
    
    glm::mat4 proj;
    glm::mat4 view;

    Shader(const glm::mat4 proj, const glm::mat4 view)
        :proj(proj), view(view)
    {
    
    }

    Triangle RasterizeTriangle(const Triangle& t, const glm::mat4 model){
        
        Triangle raster;
        glm::mat4 mvp{ proj * view * model };
        
        //get in clip space
        raster.v1 = mvp * glm::vec4{t.v1, 1.0f};
        raster.v2 = mvp * glm::vec4{t.v2, 1.0f};
        raster.v3 = mvp * glm::vec4{t.v3, 1.0f};

        //TODO: clip...

        //get in screen space
        raster.v1.x = (raster.v1.x + 1) * (WIDTH / 2);
        raster.v1.y = (raster.v1.y + 1) * (HEIGHT / 2);
        raster.v2.x = (raster.v2.x + 1) * (WIDTH / 2);
        raster.v2.y = (raster.v2.y + 1) * (HEIGHT / 2);
        raster.v3.x = (raster.v3.x + 1) * (WIDTH / 2);
        raster.v3.y = (raster.v3.y + 1) * (HEIGHT / 2);


        //TODO: Lerp colours
        raster.colour = t.colour;
        return raster;
    }
    

};

struct FrameBuffer{

    FrameBuffer(){
        Colours.reserve(FRAMEBUFFER_SIZE);
        Depth.reserve(FRAMEBUFFER_SIZE);
    }
    
    std::vector<glm::vec4> Colours;
    std::vector<float> Depth;

    void Clear(){
        for (int i{0}; i < Colours.size(); i++){
            Colours[i] = black;
        }
    }

    void DrawLine(const glm::vec2& v1, const glm::vec2& v2, const glm::vec4& colour){
        
        float dy{ (v1.y - v2.y) };
        float dx{ (v1.x - v2.x) };
        float step;

        if (abs(dx) > abs(dy)){
            step = abs(dx);
        }
        else step = abs(dy);
        
        float xInc { dx / step };
        float yInc { dy / step };

        for (int i{ 0 }; i <= step; i++){
            int xIndex { static_cast<int>(round(v2.x + (xInc * i))) };
            int yIndex { static_cast<int>(round(v2.y + (yInc * i))) };

            //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
            if (xIndex < 0 || xIndex >= WIDTH || yIndex < 0 || yIndex >= HEIGHT) continue;

            Colours[xIndex + (yIndex * WIDTH)] = colour;
        }
    }

    void DrawTriangle(const Triangle& t){
        DrawLine(glm::vec2{t.v1.x, t.v1.y}, glm::vec2{t.v2.x, t.v2.y}, t.colour);
        DrawLine(glm::vec2{t.v2.x, t.v2.y}, glm::vec2{t.v3.x, t.v3.y}, t.colour);
        DrawLine(glm::vec2{t.v3.x, t.v3.y}, glm::vec2{t.v1.x, t.v1.y}, t.colour);

        //TODO: FILL POLYGON
    }

    

};

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    std::cout << "Window width: " << WIDTH << ". Window height: " << HEIGHT << '\n';
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(WIDTH, HEIGHT, "Rasterizer", NULL, NULL);
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

    FrameBuffer frameBuff;

    for (int i{0}; i < (WIDTH*HEIGHT); i++){
        frameBuff.Colours.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    glm::mat4 proj { glm::perspective(110.0f, static_cast<float>(WIDTH)/HEIGHT, 0.01f, 10000.0f) };
    glm::mat4 view { 1.0f };
    glm::translate(view, glm::vec3{0.0f, 0.0f, -200.0f});

    Shader shader{proj, view};


    Triangle JASH{
     glm::vec3 { 50.0f,  50.0f,  0.0f },
     glm::vec3 { 250.0f, 150.0f, 0.0f },
     glm::vec3 { 450.0f, 50.0f,  0.0f },
     yellow
    };

    Triangle SEB{
     glm::vec3 { 5.0f,  5.0f,   0.0f },
     glm::vec3 { 50.0f, 300.0f, 0.0f },
     glm::vec3 { 37.0f, 12.0f,  0.0f },
     green
    };

    Triangle TOMMY{
     glm::vec3 { 3.0f, 1.0f , 0.0f },
     glm::vec3 { 6.0f, 9.0f , 0.0f },
     glm::vec3 { 4.0f, 20.0f, 0.0f },
     burgundy
    };


    glm::mat4 model { 1.0f };

    float rotation = 0;
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        frameBuff.Clear();

        //input
        glfwPollEvents();

        rotation += 0.005f;
        model = glm::rotate(glm::mat4{1.0f}, rotation, glm::vec3(1.0f, 0.0f, 0.0f));

        //render
        frameBuff.DrawTriangle(shader.RasterizeTriangle(JASH, model));
        frameBuff.DrawTriangle(shader.RasterizeTriangle(SEB, model));
        frameBuff.DrawTriangle(shader.RasterizeTriangle(TOMMY, model));


        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
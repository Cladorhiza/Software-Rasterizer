
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"

#include "InputManager.h"

#include <iostream>
#include <vector>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
constexpr int FRAMEBUFFER_SIZE = WIDTH * HEIGHT;
constexpr float CLIP_FAR = 500.0f;
constexpr float CLIP_NEAR = 1.0f;
constexpr float FOV = 110.f;

const glm::vec4 BLACK{0.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 RED{1.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 GREEN{0.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 BLUE{0.0f, 0.0f, 1.0f, 1.0f};
const glm::vec4 YELLOW{1.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 BURGUNDY{0.5f, 0.0f, 0.125f, 1.0f};

struct Triangle{

    Triangle()
        : v1(), v2(), v3(), colour()
    {
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

    Triangle RasterizeTriangle(const Triangle& t, const glm::mat4& model){
        
        //get in view space
        Triangle mv;
        glm::mat4 modelView { view * model };

        mv.v1 = modelView * glm::vec4{t.v1, 1.0f};
        mv.v2 = modelView * glm::vec4{t.v2, 1.0f};
        mv.v3 = modelView * glm::vec4{t.v3, 1.0f};

        //get in clip space
        Triangle clip;

        glm::vec4 c1, c2, c3;

        c1 = proj * glm::vec4{mv.v1, 1.0f};
        c2 = proj * glm::vec4{mv.v3, 1.0f};
        c3 = proj * glm::vec4{mv.v2, 1.0f};

        //TODO: clip...
        //if (clip.v1.z < CLIP_NEAR || clip.v1.z > CLIP_FAR
        // || clip.v2.z < CLIP_NEAR || clip.v2.z > CLIP_FAR
        // || clip.v3.z < CLIP_NEAR || clip.v3.z > CLIP_FAR) {
        //    std::cout << "clipped on the z owo\n";
        //    return Triangle{};
        //}
        
        clip.v1 = c1 / c1.w;
        clip.v2 = c2 / c2.w;
        clip.v3 = c3 / c3.w;

        //get in screen space
        Triangle screen;
        screen.v1.x = (clip.v1.x + 1) * (WIDTH / 2);
        screen.v1.y = (clip.v1.y + 1) * (HEIGHT / 2);
        screen.v2.x = (clip.v2.x + 1) * (WIDTH / 2);
        screen.v2.y = (clip.v2.y + 1) * (HEIGHT / 2);
        screen.v3.x = (clip.v3.x + 1) * (WIDTH / 2);
        screen.v3.y = (clip.v3.y + 1) * (HEIGHT / 2);
        
        screen.v1.z = clip.v1.z;
        screen.v2.z = clip.v2.z;
        screen.v3.z = clip.v3.z;

        //TODO: Lerp colours
        screen.colour = t.colour;
        return screen;
    }
};

struct FrameBuffer{

    FrameBuffer(){
        Colours.resize(FRAMEBUFFER_SIZE);
        Depth.resize(FRAMEBUFFER_SIZE);
    }
    
    std::vector<glm::vec4> Colours;
    std::vector<float> Depth;

    void Clear(){
        for (int i{0}; i < FRAMEBUFFER_SIZE; i++){
            Colours[i] = BLACK;
        }
        //in separate loops for cache locality.. at least that's the idea
    	for (int i{0}; i < FRAMEBUFFER_SIZE; i++){
            Depth[i] = -1.0f;
        }
    }

    void DrawLine(const glm::vec3& v1, const glm::vec3& v2, const glm::vec4& colour){
        
        float dx{ v1.x - v2.x };
        float dy{ v1.y - v2.y };
        float dz{ v1.z - v2.z };
        float step;

        if (abs(dx) > abs(dy)){
            step = abs(dx);
        }
        else step = abs(dy);
        
        float xInc { dx / step };
        float yInc { dy / step };
        float zInc { dz / step };
        float z { v2.z };

        for (int i{ 0 }; i <= step; i++){
            int xIndex { static_cast<int>(round(v2.x + (xInc * i))) };
            int yIndex { static_cast<int>(round(v2.y + (yInc * i))) };
            int buffIndex { xIndex + yIndex * WIDTH };                                                                                     
            
            z += zInc;
            //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
            if (xIndex < 0 || xIndex >= WIDTH || yIndex < 0 || yIndex >= HEIGHT /* || z < -1.0f || z > 1.0f || Depth[buffIndex] < z*/) continue;

            Depth[buffIndex] = z;
            
            Colours[buffIndex] = colour;
        }
    }

    void DrawTriangle(const Triangle& t){
        DrawLine(t.v1, t.v2, t.colour);
        DrawLine(t.v2, t.v3, t.colour);
        DrawLine(t.v3, t.v1, t.colour);

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
    InputManager::Init(window);

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

	memset(frameBuff.Colours.data(), 0, FRAMEBUFFER_SIZE * sizeof(glm::vec4));

    for (int i{0}; i < (FRAMEBUFFER_SIZE); i++){
        frameBuff.Depth[i] = -1;
    }
    
    glm::mat4 proj { glm::perspective(FOV, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR) };
    glm::mat4 view { 1.0f };

    Shader shader{proj, view};


    Triangle tri1{
     glm::vec3 { -200.0f,  -50.0f,  0.0f },
     glm::vec3 { 0.0f, 50.0f, 0.0f },
     glm::vec3 { 200.0f, -50.0f,  0.0f },
     YELLOW
    };

    Triangle tri2{
     glm::vec3 { 25.f,  25.0f,   25.0f },
     glm::vec3 { 25.0f, 25.0f, -25.0f },
     glm::vec3 { -50.0f, -50.0f,  0.0f },
     GREEN
    };


    float perspective = FOV;

    glm::mat4 model { 1.0f };

    float rotation { 0 };

    glm::vec3 camRotation { 0.0f, 0.0f, 0.0f };
    glm::vec3 camTranslation {0.0f, 0.0f, -5.0f};
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        frameBuff.Clear();

        //input
        InputManager::Poll(window);

        if (InputManager::GetKeyState(GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        rotation += 0.005f;
        //model = glm::rotate(glm::mat4{ 1.0f }, rotation, glm::vec3{0.0f, 1.0f, 0.0f});
        
        //camera
        if (InputManager::GetKeyState(GLFW_KEY_A) == GLFW_PRESS){
            camTranslation.x += 0.5f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_D) == GLFW_PRESS){
            camTranslation.x -= 0.5f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_W) == GLFW_PRESS){
            camTranslation.y -= 0.5f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_S) == GLFW_PRESS){
            camTranslation.y += 0.5f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_X) == GLFW_PRESS){
        	camTranslation.z -= 5.f;
            std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
            //perspective += 0.001f;
            //shader.proj = glm::perspective(perspective, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR);
            //std::cout << "perspective: " << perspective << '\n';
        }
        if (InputManager::GetKeyState(GLFW_KEY_C) == GLFW_PRESS){
            camTranslation.z += 5.f;
            std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
            //perspective -= 0.001f;
            //shader.proj = glm::perspective(perspective, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR);
            //std::cout << "perspective: " << perspective << '\n';
        }
        if (InputManager::GetKeyState(GLFW_KEY_Q) == GLFW_PRESS){
            camRotation.y -= 0.005f;
            //std::cout << camRotation.y << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_E) == GLFW_PRESS){
            camRotation.y += 0.005f;
            //std::cout << camRotation.y << "\n";
        }
        
        shader.view = glm::mat4{1.0f};
        glm::mat4 rotation {glm::rotate(glm::mat4{ 1.0f }, camRotation.y, glm::vec3{0.0f, 1.0f, 0.0f})};
        glm::mat4 translation {glm::translate(glm::mat4{ 1.0f }, camTranslation)};
        
        shader.view = rotation * translation * shader.view;

        //render
        frameBuff.DrawTriangle(shader.RasterizeTriangle(tri1, model));
        //frameBuff.DrawTriangle(shader.RasterizeTriangle(tri2, model));


        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
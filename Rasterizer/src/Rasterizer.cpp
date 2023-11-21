
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/rotate_vector.hpp"

#include "InputManager.h"

#include <iostream>
#include <vector>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
//triangle drawing cannot draw larger than 2^16-1
static_assert(WIDTH < (2 << 15) - 1);
static_assert(HEIGHT < (2 << 15) - 1);

constexpr int FRAMEBUFFER_SIZE = WIDTH * HEIGHT;
constexpr float CLIP_FAR = 500.0f;
constexpr float CLIP_NEAR = 1.0f;
constexpr float FOV = glm::radians(90.0f);

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

    Triangle ToScreenSpace(Triangle t, const glm::mat4& model){
        
        glm::mat4 mvp { proj * view * model };
        
        //get in clip space
        glm::vec4 v1, v2, v3;

        v1 = mvp * glm::vec4{t.v1, 1.0f};
        v2 = mvp * glm::vec4{t.v2, 1.0f};
        v3 = mvp * glm::vec4{t.v3, 1.0f};
        
        //if depth from origin in clip space is 0, I'm setting it to -1 to make sure they don't render
        if (v1.w == 0) v1.w = -1;
        if (v2.w == 0) v2.w = -1;
        if (v3.w == 0) v3.w = -1;

        t.v1 = v1 / v1.w;
        t.v2 = v2 / v2.w;
        t.v3 = v3 / v3.w;

        //get in screen space
        t.v1.x = (t.v1.x + 1) * (WIDTH / 2);
        t.v1.y = (t.v1.y + 1) * (HEIGHT / 2);
        t.v2.x = (t.v2.x + 1) * (WIDTH / 2);
        t.v2.y = (t.v2.y + 1) * (HEIGHT / 2);
        t.v3.x = (t.v3.x + 1) * (WIDTH / 2);
        t.v3.y = (t.v3.y + 1) * (HEIGHT / 2);

        //keep z ordering info from clip space
        t.v1.z = v1.z;
        t.v2.z = v2.z;
        t.v3.z = v3.z;

        //TODO: Lerp colours
        t.colour = t.colour;
        return t;
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
            Depth[i] = CLIP_FAR + 1.0f;
        }
    }

    /// <summary>
    /// Expects a triangle already in screen space. 
    /// Line drawing done with DDA algorithm.
    /// </summary>
    /// <param name="t">The triangle to draw to the buffer</param>
    void DrawTriangle(const Triangle& t){

        using namespace std;

        glm::vec3 verts[]{t.v1, t.v2, t.v2, t.v3, t.v3, t.v1};

        //storage for pixels drawn in specific format
        vector<pair<uint16_t,uint16_t>> xLineIndexes;
        vector<pair<float,float>> xLineDepths;
        //information regarding bounds of the triangle, want the bounding box to also be bounded by the window dimensions as there's no reason to store x or y values outside of the screen
        int xUpper { min(static_cast<int>(max({round(t.v1.x), round(t.v2.x), round(t.v3.x)})), WIDTH - 1) };
        int yUpper { min(static_cast<int>(max({round(t.v1.y), round(t.v2.y), round(t.v3.y)})), HEIGHT - 1)};
        int xLower { max(static_cast<int>(min({round(t.v1.x), round(t.v2.x), round(t.v3.x)})), 0) };
        int yLower { max(static_cast<int>(min({round(t.v1.y), round(t.v2.y), round(t.v3.y)})), 0) };
        //each vector represents the triangles x points on the lines of the triangle
        xLineIndexes.resize(abs(yUpper - yLower) + 1);
        xLineDepths.resize(abs(yUpper - yLower) + 1);
        //ensure that any x value present will be less than the initial minimum x
        for (auto& [xMin, _ ] : xLineIndexes){
            xMin = WIDTH - 1;
        }
        //make larger than possible to mark invalid values
        for (auto& [xDMin, xDMax ] : xLineDepths){
            xDMin = CLIP_FAR + 1.0f;
            xDMax = CLIP_NEAR - 1.0f;
        }

        for (int i {0}; i < 6; i+=2){
        
            glm::vec3 v1 { verts[i] };
            glm::vec3 v2 { verts[i+1] };

            float dx{ v1.x - v2.x };
            float dy{ v1.y - v2.y };
            float dz{ v1.z - v2.z };
            float step;
            
            if (abs(dx) > abs(dy)) step = abs(dx);
            else step = abs(dy);
                        
            float xInc { (step == 0)? 0 : dx / step };
            float yInc { (step == 0)? 0 : dy / step };
            float zInc { (step == 0)? 0 : dz / step };
            float z { v2.z };

            for (int i{ 0 }; i <= step; i++){

                //TODO: potential floating point errors causing indexes to be out of bounds
                uint16_t xIndex { static_cast<uint16_t>(round(v2.x + (xInc * i))) };
                uint16_t yIndex { static_cast<uint16_t>(round(v2.y + (yInc * i))) };
                int buffIndex { xIndex + yIndex * WIDTH };

                int lineValIndex { yIndex - yLower };
                z += zInc;

                //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
                if (yIndex < 0 || yIndex >= HEIGHT) continue;

                pair<uint16_t,uint16_t>& xLine { xLineIndexes[lineValIndex] };
                pair<float,float>& xLineD { xLineDepths[lineValIndex] };

                //TODO: depth won't work properly for partial triangles, because these depth values are for the offscreen coordinate, not the one I'm replacing it with
                if (xIndex < 0) {
                    xLine.first = 0;
                    xLineD.first = z;
                    continue;
                }
                else if (xIndex >= WIDTH) {
                    xLine.second = WIDTH-1;
                    xLineD.second = z;
                    continue;
                }

                xLine.first = min(xIndex, xLine.first);
                xLine.second = max(xIndex, xLine.second);
                xLineD.first = min(z, xLineD.first);
                xLineD.second = max(z, xLineD.second);

                //TODO: check depth values are less than near clip plane/far plane
                if (Depth[buffIndex] < z) continue;

                Depth[buffIndex] = z;
            
                Colours[buffIndex] = t.colour;
            }
        }

        //FILL POLYGON
        for (int i { 0 }; i < xLineIndexes.size(); i++){
            
            int buffIndexStart { xLineIndexes[i].first };
            int buffIndexEnd { xLineIndexes[i].second };

            float zStart { xLineDepths[i].first };
            float zEnd { xLineDepths[i].second };
            float zInterp { zStart };
            float zStep { (zEnd - zStart) / (buffIndexEnd - buffIndexStart) }; //TODO: potential div by 0

            for (int j { buffIndexStart + 1 }; j < buffIndexEnd; j++){
                
                int temp { j + ((i + yLower) * WIDTH) };

                //write pixel if depth is lower
                if (Depth[temp] > zInterp) {
                    
                    Depth[temp] = zInterp;
                    zInterp += zStep;
                    
                    Colours[temp] = t.colour;
                }

                
            }
        }
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
    //glm::mat4 proj { glm::ortho(-100.0f, 100.0f, 100.0f, 100.0f)};
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

    glm::vec3 camForward { 0.0f, 0.0f, -1.0f };
    glm::vec3 camTranslation {0.0f, 0.0f, 200.0f};
    
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
            camTranslation.x -= 5.f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_D) == GLFW_PRESS){
            camTranslation.x += 5.f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_W) == GLFW_PRESS){
            camTranslation.y += 5.f;
            //std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_S) == GLFW_PRESS){
            camTranslation.y -= 5.f;
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
            camForward = glm::rotate(camForward, 0.05f, glm::vec3{0.0f, 1.0f, 0.0f});
            //std::cout << camRotation.y << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_E) == GLFW_PRESS){
            camForward = glm::rotate(camForward, -0.05f, glm::vec3{0.0f, 1.0f, 0.0f});
            //std::cout << camRotation.y << "\n";
        }
        
        shader.view = glm::lookAt(camTranslation, camTranslation + camForward, {0.0f, 1.0f, 0.0f});

        //render
        frameBuff.DrawTriangle(shader.ToScreenSpace(tri1, model));
        frameBuff.DrawTriangle(shader.ToScreenSpace(tri2, model));

        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
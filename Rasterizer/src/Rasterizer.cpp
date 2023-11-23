
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/rotate_vector.hpp"

#include "InputManager.h"

#include <iostream>
#include <vector>
#include <algorithm>

constexpr int WIDTH = 960;
constexpr int HEIGHT = 540;
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

const glm::vec4 CLEAR_COLOUR{BLACK};

//Linear interpolation
inline float Lerp(float x1, float x2, float ratio) {
    float range = x2 - x1;
    return x1 + (range * ratio);
}

//barycentric weights TODO: vec2 overload
inline glm::vec3 BWeights(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& p){

    float det { ((v2.y - v3.y)*(v1.x - v3.x) + (v3.x - v2.x)*(v1.y - v3.y)) };
    //TODO: dunno if any other fix better
    if (det == 0.0f) det == 0.00000000001f;

    float w1 { ((v2.y - v3.y)*(p.x - v3.x) + (v3.x - v2.x)*(p.y - v3.y)) / det };
    float w2 { ((v3.y - v1.y)*(p.x - v3.x) + (v1.x - v3.x)*(p.y - v3.y)) / det };
    float w3 { 1 - w1 - w2 };

    //barycentric weights must add up to essentially 1
    //TODO: When looking at a tri side on, it's screenspace coords could have x and/or y being the same for all points, seems to cause problems.
    return { w1, w2, w3};
}


struct Triangle{

    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 v3;
    glm::vec4 c1;
    glm::vec4 c2;
    glm::vec4 c3;

    Triangle()
        : v1(), v2(), v3(), c1(), c2(), c3()
    {
    }

    Triangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec4& c1, const glm::vec4& c2, const glm::vec4& c3)
        : v1(v1), v2(v2), v3(v3), c1(c1), c2(c2), c3(c3)
    {
    }
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
            Colours[i] = CLEAR_COLOUR;
        }
        //in separate loops for cache locality.. at least that's the idea
    	for (int i{0}; i < FRAMEBUFFER_SIZE; i++){
            Depth[i] = CLIP_FAR;
        }
    }

    inline void DrawPixel(const Triangle& t, glm::vec2 pixelPosition, int bufferIndex){
    
        glm::vec3 weights { BWeights(t.v1, t.v2, t.v3, {pixelPosition, 0.0f}) };
        float pixelDepth { t.v1.z * weights.x + t.v2.z * weights.y + t.v3.z * weights.z };

        //write pixel if depth is lower
        if (Depth[bufferIndex] > pixelDepth) {
                    
            Depth[bufferIndex] = pixelDepth;
                     
            //TODO: these weights are calc'd from rounded coordinates, resulting in potentially inaccurate values
            Colours[bufferIndex] = glm::vec4{t.c1 * weights.x + t.c2 * weights.y + t.c3 * weights.z};
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
        //information regarding bounds of the triangle, want the bounding box to also be bounded by the window dimensions as there's no reason to store x or y values outside of the screen
        int xUpper { min(static_cast<int>(max({floor(t.v1.x), floor(t.v2.x), floor(t.v3.x)})), WIDTH - 1) };
        int yUpper { min(static_cast<int>(max({floor(t.v1.y), floor(t.v2.y), floor(t.v3.y)})), HEIGHT - 1)};
        int xLower { max(static_cast<int>(min({floor(t.v1.x), floor(t.v2.x), floor(t.v3.x)})), 0) };
        int yLower { max(static_cast<int>(min({floor(t.v1.y), floor(t.v2.y), floor(t.v3.y)})), 0) };
        //each vector represents the triangles x points on the lines of the triangle
        xLineIndexes.resize(abs(yUpper - yLower) + 1);
        //ensure that any x value present will be less than the initial minimum x
        for (auto& [xMin, _ ] : xLineIndexes){
            xMin = WIDTH - 1;
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

            for (int i{ 0 }; i <= step; i++){

                //TODO: potential floating point/overflow errors causing indexes to be out of bounds
                uint16_t xIndex { static_cast<uint16_t>(floor(v2.x + (xInc * i))) };
                uint16_t yIndex { static_cast<uint16_t>(floor(v2.y + (yInc * i))) };
                int buffIndex { xIndex + yIndex * WIDTH };

                int lineValIndex { yIndex - yLower };

                //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
                if (yIndex < 0 || yIndex >= HEIGHT) continue;

                //TODO: overflows at high values
                pair<uint16_t,uint16_t>& xLine { xLineIndexes[lineValIndex] };

                //TODO: depth won't work properly for partial triangles, because these depth values are for the offscreen coordinate, not the one I'm replacing it with
                if (xIndex < 0) {
                    xLine.first = 0;
                    continue;
                }
                else if (xIndex >= WIDTH) {
                    xLine.second = WIDTH-1;
                    continue;
                }

                xLine.first = min(xIndex, xLine.first);
                xLine.second = max(xIndex, xLine.second);

                DrawPixel(t, {xIndex, yIndex}, buffIndex);
            }
        }

        //FILL POLYGON
        for (int i { 0 }; i < xLineIndexes.size(); i++){
            
            int buffIndexStart { xLineIndexes[i].first };
            int buffIndexEnd { xLineIndexes[i].second };

            for (int j { buffIndexStart + 1 }; j < buffIndexEnd; j++){
                
                int colourIndex { j + ((i + yLower) * WIDTH) };

                DrawPixel(t, { j, i + yLower }, colourIndex);
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
        frameBuff.Depth[i] = CLIP_FAR;
    }
    
    glm::mat4 proj { glm::perspective(FOV, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR) };
    //glm::mat4 proj { glm::ortho(-100.0f, 100.0f, 100.0f, 100.0f)};
    glm::mat4 view { 1.0f };

    Shader shader{proj, view};


    Triangle tris[]{
        //front
{
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        GREEN,
        BLUE,
        RED,
},
{

        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        GREEN,
        BLUE,
        RED,
},
        //left
{
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
     
{
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
        //right
{
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f,-100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
     
{
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
        //top
{
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
     
{
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f,-100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
        //bottom
{
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
     
{
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        GREEN,
        BLUE,
        RED,
},
        //back
{
        glm::vec3 { 100.0f, 100.0f, -100.0f },
        glm::vec3 { 100.0f,-100.0f, -100.0f },
        glm::vec3 {-100.0f,-100.0f, -100.0f },
        GREEN,
        BLUE,
        RED,
},
     
{
        glm::vec3 { 100.0f, 100.0f, -100.0f },
        glm::vec3 {-100.0f, 100.0f, -100.0f },
        glm::vec3 {-100.0f,-100.0f, -100.0f },
        GREEN,
        BLUE,
        RED
}
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

        rotation += 0.05f;
        model = glm::translate(glm::mat4{ 1.0f }, {0.0f, 100.f*sin(rotation), 0.0f});
        model = glm::rotate(model, rotation, glm::vec3{0.0f, 1.0f, 0.0f});
        
        //camera
        if (InputManager::GetKeyState(GLFW_KEY_A) == GLFW_PRESS){
            camTranslation.x -= 5.f;
        }
        if (InputManager::GetKeyState(GLFW_KEY_D) == GLFW_PRESS){
            camTranslation.x += 5.f;
        }
        if (InputManager::GetKeyState(GLFW_KEY_W) == GLFW_PRESS){
            camTranslation.y += 5.f;
        }
        if (InputManager::GetKeyState(GLFW_KEY_S) == GLFW_PRESS){
            camTranslation.y -= 5.f;
        }
        if (InputManager::GetKeyState(GLFW_KEY_X) == GLFW_PRESS){
        	camTranslation.z -= 5.f;
            std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_C) == GLFW_PRESS){
            camTranslation.z += 5.f;
            std::cout << camTranslation.x << ", " << camTranslation.y << ", " << camTranslation.z << "\n";
        }
        if (InputManager::GetKeyState(GLFW_KEY_Q) == GLFW_PRESS){
            camForward = glm::rotate(camForward, 0.05f, glm::vec3{0.0f, 1.0f, 0.0f});
        }
        if (InputManager::GetKeyState(GLFW_KEY_E) == GLFW_PRESS){
            camForward = glm::rotate(camForward, -0.05f, glm::vec3{0.0f, 1.0f, 0.0f});
        }
        
        shader.view = glm::lookAt(camTranslation, camTranslation + camForward, {0.0f, 1.0f, 0.0f});

        //render
        for (int i { 0 }; i < 12; i++){
            frameBuff.DrawTriangle(shader.ToScreenSpace(tris[i], model));
        }

        //glDrawPixels(WIDTH, HEIGHT, GL_RED, GL_FLOAT, frameBuff.Depth.data());
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
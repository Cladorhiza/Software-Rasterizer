//thirdparty
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/rotate_vector.hpp"
//proj
#include "InputManager.h"
#include "CoreTypes.h"
//stl
#include <iostream>
#include <vector>
#include <algorithm>

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
//triangle drawing cannot draw larger than 2^16-1
static_assert(WIDTH < (2 << 15) - 1);
static_assert(HEIGHT < (2 << 15) - 1);

constexpr float CLIP_FAR = 1000.0f;
constexpr float CLIP_NEAR = 1.0f;
constexpr float FOV = glm::radians(90.0f);

const glm::vec4 WHITE(1.0f, 1.0f, 1.0f, 1.0f);
const glm::vec4 BLACK{0.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 RED{1.0f, 0.0f, 0.0f, 1.0f};
const glm::vec4 GREEN{0.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 BLUE{0.0f, 0.0f, 1.0f, 1.0f};
const glm::vec4 YELLOW{1.0f, 1.0f, 0.0f, 1.0f};
const glm::vec4 BURGUNDY{0.5f, 0.0f, 0.125f, 1.0f};

const glm::vec4 CLEAR_COLOUR{BLACK};

Triangle tris[]{
    //front
    {
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {1.0f, 0.0f},
        glm::vec2 {1.0f, 1.0f}
    },
    {
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {1.0f, 1.0f}
    },
    //left
    {
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {1.0f, 1.0f},
        glm::vec2 {1.0f, 0.0f},
        glm::vec2 {0.0f, 0.0f}
    },
    {
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {1.0f, 1.0f},
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {0.0f, 0.0f}
    },
    //right
    {
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f,-100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {1.0f, 1.0f},
        glm::vec2 {1.0f, 0.0f}
    },
    {
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {1.0f, 0.0f}
    },
    //back
    {
        glm::vec3 { 100.0f, 100.0f, -100.0f },
        glm::vec3 { 100.0f,-100.0f, -100.0f },
        glm::vec3 {-100.0f,-100.0f, -100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {1.0f, 0.0f}
    },
    {
        glm::vec3 { 100.0f, 100.0f, -100.0f },
        glm::vec3 {-100.0f, 100.0f, -100.0f },
        glm::vec3 {-100.0f,-100.0f, -100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {1.0f, 1.0f},
        glm::vec2 {1.0f, 0.0f}
    },
    //top
    {
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f, 100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {1.0f, 0.0f},
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {0.0f, 1.0f}
    },
    {
        glm::vec3 { 100.0f, 100.0f, 100.0f },
        glm::vec3 { 100.0f, 100.0f,-100.0f },
        glm::vec3 {-100.0f, 100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {1.0f, 0.0f},
        glm::vec2 {1.0f, 1.0f},
        glm::vec2 {0.0f, 1.0f}
    },
    //bottom
    {
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 { 100.0f,-100.0f,-100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {0.0f, 1.0f},
        glm::vec2 {1.0f, 1.0f}
    },
    {
        glm::vec3 { 100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f, 100.0f },
        glm::vec3 {-100.0f,-100.0f,-100.0f },
        WHITE,
        WHITE,
        WHITE,
        glm::vec2 {0.0f, 0.0f},
        glm::vec2 {1.0f, 0.0f},
        glm::vec2 {1.0f, 1.0f}
    }
};

int main(void)
{
    //Init
    GLFWwindow* window;

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

    //~Init


    //Application

    FrameBuffer frameBuff { WIDTH, HEIGHT };
    frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);
    
    glm::mat4 proj { glm::perspective(FOV, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR) };
    glm::mat4 view { 1.0f };

    Shader shader{proj, view};

    Texture wood_front{ "res/textures/wood_side.png" };
    Texture wood_top{ "res/textures/wood_top.png" };

    glm::mat4 model { 1.0f };

    float rotation { 0 };

    glm::vec3 camForward { 0.0f, 0.0f, -1.0f };
    glm::vec3 camTranslation {0.0f, 0.0f, 200.0f};
    
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);

        //input
        InputManager::Poll(window);

        if (InputManager::GetKeyState(GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        rotation += 0.075f;
        if (rotation > 360.0f) rotation -= 360.0f;
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
            if (i < 8) frameBuff.DrawTriangle(shader.ToClipSpace(tris[i], model), wood_front); // draw log sides
            else frameBuff.DrawTriangle(shader.ToClipSpace(tris[i], model), wood_top); // draw log top/bottom
        }

        //glDrawPixels(WIDTH, HEIGHT, GL_RED, GL_FLOAT, frameBuff.Depth.data());
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
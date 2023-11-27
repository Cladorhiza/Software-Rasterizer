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
#include "FileParsing.h"
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

constexpr glm::vec4 WHITE(1.0f, 1.0f, 1.0f, 1.0f);
constexpr glm::vec4 BLACK{0.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 RED{1.0f, 0.0f, 0.0f, 1.0f};
constexpr glm::vec4 GREEN{0.0f, 1.0f, 0.0f, 1.0f};
constexpr glm::vec4 BLUE{0.0f, 0.0f, 1.0f, 1.0f};
constexpr glm::vec4 YELLOW{1.0f, 1.0f, 0.0f, 1.0f};
constexpr glm::vec4 BURGUNDY{0.5f, 0.0f, 0.125f, 1.0f};

const glm::vec4 CLEAR_COLOUR{BLACK};

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

    Model benchModel;
    FileParsing::OBJFileParseResult benchData { FileParsing::ParseObjToData(FileParsing::LoadFile("res/models/bench/Bench.obj")) };
    
    //prevent resizing
    benchModel.vertices.reserve(benchData.vertices.size()/3);
    benchModel.normals.reserve(benchData.normals.size()/3);
    benchModel.texCoords.reserve(benchData.texCoords.size()/2);
    benchModel.triIndexes.reserve(benchData.indexes.size()/3);

    for (int i {0}; i+2 < benchData.vertices.size(); i+=3){
        benchModel.vertices.emplace_back(benchData.vertices[i], benchData.vertices[i + 1], benchData.vertices[i + 2]);
    }
    for (int i {0}; i+2 < benchData.normals.size(); i+=3){
        benchModel.normals.emplace_back(benchData.normals[i], benchData.normals[i + 1], benchData.normals[i + 2]);
    }
    for (int i {0}; i+1 < benchData.texCoords.size(); i+=2){
        benchModel.texCoords.emplace_back(benchData.texCoords[i], benchData.texCoords[i + 1]);
    }
    for (int i {0}; i+2 < benchData.indexes.size(); i+=3){
        benchModel.triIndexes.emplace_back(benchData.indexes[i], benchData.indexes[i + 1], benchData.indexes[i + 2]);
    }

    FrameBuffer frameBuff { WIDTH, HEIGHT };
    frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);
    
    glm::mat4 proj { glm::perspective(FOV, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR) };
    glm::mat4 view { 1.0f };

    Shader shader{proj, view};

    Texture wood_front{ "res/textures/wood_side.png" };
    Texture wood_top{ "res/textures/wood_top.png" };
    Texture benchColour{ "res/models/bench/Bench_M1014_BaseColor.png" };

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

        //rotation += 0.075f;
        //if (rotation > 360.0f) rotation -= 360.0f;
        //model = glm::translate(glm::mat4{ 1.0f }, {0.0f, 100.f*sin(rotation), 0.0f});
        //model = glm::rotate(model, rotation, glm::vec3{0.0f, 1.0f, 0.0f});
        
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

        std::vector<Triangle> clipSpaceModel { shader.ToClipSpace(benchModel, model) };
        
        for (int i { 0 }; i < clipSpaceModel.size(); i++){
            
            frameBuff.DrawTriangle(clipSpaceModel[i], benchColour);
            
        }


        //glDrawPixels(WIDTH, HEIGHT, GL_RED, GL_FLOAT, frameBuff.Depth.data());
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
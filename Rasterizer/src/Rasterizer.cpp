//thirdparty
#include "glew.h"
#include "gl/GL.h"
#include "glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/rotate_vector.hpp"
#include "imgui.h"
//proj
#include "InputManager.h"
#include "CoreTypes.h"
#include "FileParsing.h"
#include "Shader.h"
#include "Stopwatch.h"
#include "Camera.h"
//stl
#include <iostream>
#include <vector>
#include <algorithm>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

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
    glfwSwapInterval(0); /*disable vsync*/

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    //~Init
    //Application

    
    FileParsing::OBJFileParseResult benchData { FileParsing::ParseObjToData(FileParsing::LoadFile("res/models/bench/Bench.obj")) };
    Model benchModel { Model::OBJFPRtoModel(benchData) };
    

    FrameBuffer frameBuff { WIDTH, HEIGHT };
    frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);
    
    glm::mat4 proj { glm::perspective(FOV, static_cast<float>(WIDTH)/HEIGHT, CLIP_NEAR, CLIP_FAR) };
    glm::mat4 view { 1.0f };

    Shader shader{proj, view};
    shader.lightInfo.worldPosition = {300.0f, 300.0f, 300.0f};
    shader.lightInfo.ambientIntensity = {1.0f, 1.0f, 1.0f, 1.0f};
    shader.lightInfo.diffuseIntensity = {1.0f, 1.0f, 1.0f, 1.0f};
    shader.lightInfo.specularIntensity = {1.0f, 1.0f, 1.0f, 1.0f};

    Texture wood_front{ "res/textures/wood_side.png" };
    Texture wood_top{ "res/textures/wood_top.png" };
    Texture benchColour{ "res/models/bench/Bench_M1014_BaseColor.png" };

    glm::mat4 model { 1.0f };

    float rotation { 0 };

    Camera mainCam{{0.0f, 0.0f, 200.0f}, {0.0f, 0.0f, 0.0f}};
    float cameraSpeed { 150.0f };
    float cameraRotationSpeed { 50.0f };

    Stopwatch frameTime;
    float deltaTime { 0.0 };
    //imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 150";
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool showLightWindow = false;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        frameTime.Restart();
        frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);

        //Input
        InputManager::Poll(window);

        //Logic
        if (InputManager::GetKeyState(GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        //rotation += 0.075f;
        //if (rotation > 360.0f) rotation -= 360.0f;
        //model = glm::translate(glm::mat4{ 1.0f }, {0.0f, 25.f*sin(rotation), 0.0f});
        //model = glm::rotate(model, rotation, glm::vec3{0.0f, 1.0f, 0.0f});
        
        //camera
        if (InputManager::GetKeyState(GLFW_KEY_Q) ==     GLFW_PRESS) mainCam.Translate(-mainCam.GetUp() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_E) ==     GLFW_PRESS) mainCam.Translate(mainCam.GetUp() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_W) ==     GLFW_PRESS) mainCam.Translate(mainCam.GetForward() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_A) ==     GLFW_PRESS) mainCam.Translate(-mainCam.GetRight() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_S) ==     GLFW_PRESS) mainCam.Translate(-mainCam.GetForward() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_D) ==     GLFW_PRESS) mainCam.Translate(mainCam.GetRight() * cameraSpeed * deltaTime);
        if (InputManager::GetKeyState(GLFW_KEY_UP) ==    GLFW_PRESS) mainCam.Rotate(glm::vec3{cameraRotationSpeed * deltaTime, 0.0f, 0.0f});
        if (InputManager::GetKeyState(GLFW_KEY_DOWN) ==  GLFW_PRESS) mainCam.Rotate(glm::vec3{-cameraRotationSpeed * deltaTime, 0.0f, 0.0f});
        if (InputManager::GetKeyState(GLFW_KEY_LEFT) ==  GLFW_PRESS) mainCam.Rotate(glm::vec3{0.0f, cameraRotationSpeed * deltaTime, 0.0f});
        if (InputManager::GetKeyState(GLFW_KEY_RIGHT) == GLFW_PRESS) mainCam.Rotate(glm::vec3{0.0f, -cameraRotationSpeed * deltaTime, 0.0f});
        shader.view = mainCam.GetViewMatrix();
       
        //toggle light edit debug window
        if (InputManager::GetKeyToggle(GLFW_KEY_F2)){
            showLightWindow = !showLightWindow;
        }
        

        //Render

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (showLightWindow){

            //light
            ImGui::Begin("Light Settings");
            ImGui::SliderFloat3("Position", &shader.lightInfo.worldPosition.x, -300.0f, 300.0f);            
            ImGui::ColorEdit4("Ambient Intensity", &shader.lightInfo.ambientIntensity.x);            
            ImGui::ColorEdit4("Diffuse Intensity", &shader.lightInfo.diffuseIntensity.x);            
            ImGui::ColorEdit4("Specular Intensity", &shader.lightInfo.specularIntensity.x);            
            //framerate
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        ImGui::Render();

        shader.DrawModel(benchModel, model, frameBuff, benchColour);

        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        //want in seconds
        deltaTime = frameTime.GetElapsed() / 1000000000.0;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
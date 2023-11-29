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
    Shader::LightInfo lightInfo;
    lightInfo.worldPosition = {500.0f, 500.0f, 500.0f};
    shader.lightInfo = lightInfo;

    Texture wood_front{ "res/textures/wood_side.png" };
    Texture wood_top{ "res/textures/wood_top.png" };
    Texture benchColour{ "res/models/bench/Bench_M1014_BaseColor.png" };

    glm::mat4 model { 1.0f };

    float rotation { 0 };

    glm::vec3 camForward { 0.0f, 0.0f, -1.0f };
    glm::vec3 camTranslation {0.0f, 0.0f, 200.0f};

    Stopwatch frameTimer;
    double cumulativeFrameTime = 0.0f;
    int frameCount = 0;
    



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

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);






    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        frameTimer.Restart();
        
        frameBuff.Clear(CLEAR_COLOUR, CLIP_FAR);

        //input
        InputManager::Poll(window);

        if (InputManager::GetKeyState(GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            break;
        }

        //rotation += 0.075f;
        //if (rotation > 360.0f) rotation -= 360.0f;
        //model = glm::translate(glm::mat4{ 1.0f }, {0.0f, 25.f*sin(rotation), 0.0f});
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
        if (InputManager::GetKeyToggle(GLFW_KEY_F2)){
            show_another_window = !show_another_window;
        }
        
        shader.view = glm::lookAt(camTranslation, camTranslation + camForward, {0.0f, 1.0f, 0.0f});

        //Render

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        //if (show_demo_window)
        //    ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;
        
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
        
            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
        
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
        
            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
        
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }
        
        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        ImGui::Render();



        shader.DrawModel(benchModel, model, frameBuff, benchColour);

        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, frameBuff.Colours.data());
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        cumulativeFrameTime += frameTimer.GetElapsed();

        if (cumulativeFrameTime / 1000000000 > 1) {
            cumulativeFrameTime -= 1000000000;
            std::cout << "Fps: " << frameCount << "\n";
            frameCount = 0;
        }
        else frameCount++;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
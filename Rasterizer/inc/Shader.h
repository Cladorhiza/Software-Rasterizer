#pragma once

#include "glm.hpp"

#include "CoreTypes.h"

#include <thread>
#include <condition_variable>

struct ClipSpaceInfo{
    Triangle t;

    //lighting info
    glm::vec3 v1view, v2view, v3view;
    glm::vec3 n1view, n2view, n3view;
    glm::vec3 r1view, r2view, r3view;
};

class Shader;

struct FragmentShaderThread{
        
    std::thread thread;

    bool alive = true;
    FrameBuffer* fb;
    const Texture* tex;
    ClipSpaceInfo* csi;
    std::vector<std::pair<glm::vec2, int>> pixelCoordsWithIndex;

    void Setup(FrameBuffer& fb,
                const Texture& tex,
                ClipSpaceInfo& csi,
                std::vector<std::pair<glm::vec2, int>> pixelCoordsWithIndex);

    void DrawPixelJob(std::condition_variable& cv, std::mutex& mutex, std::atomic_int& threadsReady, Shader* shader);
};

//Shaders store (uniform) state that is required to transform a triangle from object space to clip space
class Shader{
    
    

public:

    struct LightInfo{
        glm::vec3 worldPosition;
        glm::vec4 ambientIntensity, diffuseIntensity, specularIntensity;
    };

    const int THREAD_COUNT;
    std::mutex fragmentStart;
    std::condition_variable fragmentStartNotifier;
    std::atomic_int fragmentReadyCount;
    FragmentShaderThread* fragmentShaderThreadPool;

    glm::mat4 proj;
    glm::mat4 view;
    LightInfo lightInfo;

    Shader(const glm::mat4& proj, const glm::mat4& view);

    //TODO: should my pipeline be static, with some kind of shader parameter to call vertex/fragment operations on, while keeping everything else fixed?
    ClipSpaceInfo ToClipSpace(const Triangle& t, const glm::mat4& model);
    std::vector<ClipSpaceInfo> ToClipSpace(const Model& m, const glm::mat4& model);
    void RasterizeTriangle(ClipSpaceInfo t, const Texture& tex, FrameBuffer& fb);
    void DrawPixel(const ClipSpaceInfo& t, glm::vec2 pixelPosition, int bufferIndex, const Texture& tex, FrameBuffer& fb);
    void DrawModel(const Model& model, const glm::mat4& modelMatrix, FrameBuffer& frameBuffer, const Texture& texture);

    
};
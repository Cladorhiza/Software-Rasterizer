#pragma once

#include "glm.hpp"

#include "CoreTypes.h"


//Shaders store (uniform) state that is required to transform a triangle from object space to clip space
class Shader{
    
    struct ClipSpaceInfo{
        Triangle t;

        //lighting info
        glm::vec3 v1view, v2view, v3view;
        glm::vec3 n1view, n2view, n3view;
        glm::vec3 r1view, r2view, r3view;
    };

public:

    struct LightInfo{
        glm::vec3 worldPosition;
        glm::vec4 ambientIntensity, diffuseIntensity, specularIntensity;
    };

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
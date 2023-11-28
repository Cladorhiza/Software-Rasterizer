#pragma once

#include "glm.hpp"

#include "CoreTypes.h"


//Shaders store (uniform) state that is required to transform a triangle from object space to clip space
struct Shader{
    
    glm::mat4 proj;
    glm::mat4 view;

    Shader(const glm::mat4& proj, const glm::mat4& view);

    //TODO: should my pipeline be static, with some kind of shader parameter to call vertex/fragment operations on, while keeping everything else fixed?
    Triangle ToClipSpace(Triangle t, const glm::mat4& model);
    std::vector<Triangle> ToClipSpace(const Model& m, const glm::mat4& model);
    void DrawTriangle(Triangle t, const Texture& tex, FrameBuffer& fb);
    void DrawPixel(const Triangle& t, glm::vec2 pixelPosition, int bufferIndex, const Texture& tex, FrameBuffer& fb);
};
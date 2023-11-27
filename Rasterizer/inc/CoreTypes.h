#pragma once
#include "glm.hpp"
#include "stb_image.h"

#include "Maths.h"

#include <vector>
#include <string>

struct Triangle{

    //verts
    glm::vec3 v1, v2, v3;
    //colours
    glm::vec4 c1, c2, c3;
    //texcoords
    glm::vec2 uv1, uv2, uv3;

    Triangle();

    Triangle(const glm::vec3& v1, 
             const glm::vec3& v2, 
             const glm::vec3& v3, 
             const glm::vec4& c1, 
             const glm::vec4& c2, 
             const glm::vec4& c3, 
             const glm::vec2& uv1, 
             const glm::vec2& uv2, 
             const glm::vec2& uv3);
};

struct Texture{
    
    std::vector<unsigned char> image;
    int width, height, channels;

    Texture(std::string filePath);
};

//Shaders store (uniform) state that is required to transform a triangle from object space to clip space
struct Shader{
    
    glm::mat4 proj;
    glm::mat4 view;

    Shader(const glm::mat4& proj, const glm::mat4& view);

    Triangle ToClipSpace(Triangle t, const glm::mat4& model);
};

struct FrameBuffer{

    std::vector<glm::vec4> Colours;
    std::vector<float> Depth;
    int width, height;

    FrameBuffer(int width, int height);
    
    void Clear(const glm::vec4& clearColour, float clearDepth);

    void DrawPixel(const Triangle& t, glm::vec2 pixelPosition, int bufferIndex, const Texture& tex);
    
    /// <summary>
    /// Expects a triangle already in clip space. 
    /// Line drawing done with DDA algorithm.
    /// </summary>
    /// <param name="t">The triangle to draw to the buffer</param>
    /// <param name="tex">The texture to sample from when drawing pixels for the triangle</param>
    void DrawTriangle(Triangle t, const Texture& tex);
};
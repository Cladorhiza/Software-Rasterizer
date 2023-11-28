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
    //normals
    glm::vec3 n1, n2, n3;

    Triangle();

    Triangle(const glm::vec3& v1, 
             const glm::vec3& v2, 
             const glm::vec3& v3, 
             const glm::vec4& c1, 
             const glm::vec4& c2, 
             const glm::vec4& c3, 
             const glm::vec2& uv1, 
             const glm::vec2& uv2, 
             const glm::vec2& uv3,
             const glm::vec3& n1,
             const glm::vec3& n2, 
             const glm::vec3& n3);
};

struct Model{

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> triIndexes;

    Model();

    Model(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& triIndexes);

};

struct Texture{
    
    std::vector<unsigned char> image;
    int width, height, channels;

    Texture(std::string filePath);

};

struct FrameBuffer{

    std::vector<glm::vec4> Colours;
    std::vector<float> Depth;
    int width, height;

    FrameBuffer(int width, int height);

    void Clear(const glm::vec4& clearColour, float clearDepth);

};


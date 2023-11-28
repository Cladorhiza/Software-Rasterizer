#include "CoreTypes.h"

#include "Lighting.h"

Triangle::Triangle()
    :v1(0.0f), v2(0.0f), v3(0.0f), c1(0.0f), c2(0.0f), c3(0.0f), uv1(0.0f), uv2(0.0f), uv3(0.0f), n1(0.0f), n2(0.0f), n3(0.0f)
{
}

Triangle::Triangle(const glm::vec3& v1, 
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
                   const glm::vec3& n3)
    : v1(v1), v2(v2), v3(v3), c1(c1), c2(c2), c3(c3), uv1(uv1), uv2(uv2), uv3(uv3), n1(n1), n2(n2), n3(n3)
{
}

Model::Model()
{
}

Model::Model(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& triIndexes)
    :vertices(vertices), normals(normals), texCoords(texCoords), triIndexes(triIndexes)
{
}

Texture::Texture(std::string filePath) 
    :width(0), height(0), channels(0)
{
    unsigned char* pic = stbi_load(filePath.data(), &width, &height, &channels, 0);
    image.resize(width * height * channels);
    memcpy(image.data(), pic, width * height * channels * sizeof(unsigned char));
    free(pic);
}

FrameBuffer::FrameBuffer(int width, int height)
    :width(width), height(height)
{
    Colours.resize(width * height);
    Depth.resize(width * height);
}

void FrameBuffer::Clear(const glm::vec4& clearColour, float clearDepth){
    for (int i{0}; i < width * height; i++){
        Colours[i] = clearColour;
    }
    //in separate loops for cache locality.. at least that's the idea
    for (int i{0}; i < width * height; i++){
        Depth[i] = clearDepth;
    }
}
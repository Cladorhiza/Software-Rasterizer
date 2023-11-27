#include "CoreTypes.h"

Triangle::Triangle()
    :v1(0.0f), v2(0.0f), v3(0.0f), c1(0.0f), c2(0.0f), c3(0.0f), uv1(0.0f), uv2(0.0f), uv3(0.0f) 
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
                   const glm::vec2& uv3)
    : v1(v1), v2(v2), v3(v3), c1(c1), c2(c2), c3(c3), uv1(uv1), uv2(uv2), uv3(uv3)
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

Shader::Shader(const glm::mat4& proj, const glm::mat4& view)
    :proj(proj), view(view)
{
    
}

Triangle Shader::ToClipSpace(Triangle t, const glm::mat4& model){
        
    glm::mat4 mvp { proj * view * model };
        
    //get in clip space
    glm::vec4 v1, v2, v3;

    v1 = mvp * glm::vec4{t.v1, 1.0f};
    v2 = mvp * glm::vec4{t.v2, 1.0f};
    v3 = mvp * glm::vec4{t.v3, 1.0f};
        
    //if depth from origin in clip space is 0, I'm setting it to -1 to make sure they don't render
    if (v1.w == 0) v1.w = -1;
    if (v2.w == 0) v2.w = -1;
    if (v3.w == 0) v3.w = -1;

    t.v1 = v1 / v1.w;
    t.v2 = v2 / v2.w;
    t.v3 = v3 / v3.w;

    //keep z ordering info from clip space
    t.v1.z = v1.z;
    t.v2.z = v2.z;
    t.v3.z = v3.z;

    return t;
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

void FrameBuffer::DrawPixel(const Triangle& t, glm::vec2 pixelPosition, int bufferIndex, const Texture& tex) {
        
    glm::vec3 weights { Maths::BarycentricWeights(t.v1, t.v2, t.v3, {pixelPosition, 0.0f}) };
        
    //todo: this is bandaid, pixels should not be being drawn if they are outside the triangle
    if (weights.x < 0.0f || weights.y < 0.0f || weights.z < 0.0f) return;

    float pixelDepth { t.v1.z * weights.x + t.v2.z * weights.y + t.v3.z * weights.z };

    //write pixel if depth is lower
    if (Depth[bufferIndex] > pixelDepth) {
                    
        Depth[bufferIndex] = pixelDepth;
            
        if (tex.image.size() > 0) {

            glm::vec2 temp[] {
                t.uv1 * weights.x,
                t.uv2 * weights.y,
                t.uv3 * weights.z
            };

            glm::vec2 index { temp[0] + temp[1] + temp[2] };

            int indexComponents[] {
                (static_cast<int>(floor(index.x * (tex.width-1))) * tex.channels),
                (static_cast<int>(floor(index.y * (tex.height-1))) * tex.width * tex.channels)
            };

            int sampleIndex { indexComponents[0] + indexComponents[1] };
                
            glm::vec4 texSamples {
                static_cast<float>(tex.image[sampleIndex])   / 255.0f,
                static_cast<float>(tex.image[sampleIndex+1]) / 255.0f,
                static_cast<float>(tex.image[sampleIndex+2]) / 255.0f,
                static_cast<float>(tex.image[sampleIndex+3]) / 255.0f,
            };
            Colours[bufferIndex] = texSamples;
        }
        else {
            //TODO: these weights are calc'd from rounded coordinates, resulting in potentially inaccurate values
            Colours[bufferIndex] = glm::vec4{t.c1 * weights.x + t.c2 * weights.y + t.c3 * weights.z};
        }
            
    }
}
    
void FrameBuffer::DrawTriangle(Triangle t, const Texture& tex){

    using namespace std;

    //get in screen space
    t.v1.x = (t.v1.x + 1) * (width / 2);
    t.v1.y = (t.v1.y + 1) * (height / 2);
    t.v2.x = (t.v2.x + 1) * (width / 2);
    t.v2.y = (t.v2.y + 1) * (height / 2);
    t.v3.x = (t.v3.x + 1) * (width / 2);
    t.v3.y = (t.v3.y + 1) * (height / 2);

    glm::vec3 verts[]{t.v1, t.v2, t.v2, t.v3, t.v3, t.v1};

    //storage for pixels drawn in specific format
    vector<pair<uint16_t,uint16_t>> xLineIndexes;
    //information regarding bounds of the triangle, want the bounding box to also be bounded by the window dimensions as there's no reason to store x or y values outside of the screen
    int xUpper { min(static_cast<int>(max({floor(t.v1.x), floor(t.v2.x), floor(t.v3.x)})), width - 1) };
    int yUpper { min(static_cast<int>(max({floor(t.v1.y), floor(t.v2.y), floor(t.v3.y)})), height - 1)};
    int xLower { max(static_cast<int>(min({floor(t.v1.x), floor(t.v2.x), floor(t.v3.x)})), 0) };
    int yLower { max(static_cast<int>(min({floor(t.v1.y), floor(t.v2.y), floor(t.v3.y)})), 0) };
    //each vector represents the triangles x points on the lines of the triangle
    xLineIndexes.resize(abs(yUpper - yLower) + 1);
    //ensure that any x value present will be less than the initial minimum x
    for (auto& [xMin, _ ] : xLineIndexes){
        xMin = width - 1;
    }

    for (int i {0}; i < 6; i+=2){
        
        glm::vec3 v1 { verts[i] };
        glm::vec3 v2 { verts[i+1] };

        float dx{ v1.x - v2.x };
        float dy{ v1.y - v2.y };
        float dz{ v1.z - v2.z };
        float step;
            
        if (abs(dx) > abs(dy)) step = abs(dx);
        else step = abs(dy);
                        
        float xInc { (step == 0)? 0 : dx / step };
        float yInc { (step == 0)? 0 : dy / step };

        for (int i{ 0 }; i <= step; i++){

            //TODO: potential floating point/overflow errors causing indexes to be out of bounds
            uint16_t xIndex { static_cast<uint16_t>(floor(v2.x + (xInc * i))) };
            uint16_t yIndex { static_cast<uint16_t>(floor(v2.y + (yInc * i))) };
            int buffIndex { xIndex + yIndex * width };

            int lineValIndex { yIndex - yLower };

            //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
            if (yIndex < 0 || yIndex >= height) continue;

            //TODO: overflows at high values
            pair<uint16_t,uint16_t>& xLine { xLineIndexes[lineValIndex] };

            //TODO: depth won't work properly for partial triangles, because these depth values are for the offscreen coordinate, not the one I'm replacing it with
            if (xIndex < 0) {
                xLine.first = 0;
                continue;
            }
            else if (xIndex >= width) {
                xLine.second = width-1;
                continue;
            }

            xLine.first = min(xIndex, xLine.first);
            xLine.second = max(xIndex, xLine.second);

            DrawPixel(t, {xIndex, yIndex}, buffIndex, tex);
        }
    }

    //FILL POLYGON
    for (int i { 0 }; i < xLineIndexes.size(); i++){
            
        int buffIndexStart { xLineIndexes[i].first };
        int buffIndexEnd { xLineIndexes[i].second };

        for (int j { buffIndexStart + 1 }; j < buffIndexEnd; j++){
                
            int colourIndex { j + ((i + yLower) * width) };

            DrawPixel(t, { j, i + yLower }, colourIndex, tex);
        }
    }
}

Model::Model()
{


}

Model::Model(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texCoords, const std::vector<glm::vec3>& triIndexes)
    :vertices(vertices), normals(normals), texCoords(texCoords), triIndexes(triIndexes)
{



}
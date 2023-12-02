#include "Shader.h"

#include "Lighting.h"

#include <iostream>
#include <algorithm>

struct FragmentShaderThread{
        
    std::thread thread;

    bool alive = true;
    FrameBuffer* fb;
    const Texture* tex;
    ClipSpaceInfo* csi;
    std::pair<glm::vec2, int>* pixelArr;
    int pixelArrSize;

    void Setup(FrameBuffer& fb,
                const Texture& tex,
                ClipSpaceInfo& csi,
                std::pair<glm::vec2, int>* pixelArr,
                int pixelArrSize)
    {
        this->fb = &fb;
        this->tex = &tex;
        this->csi = &csi;
        this->pixelArr = pixelArr;
        this->pixelArrSize = pixelArrSize;
    }

    void DrawPixelJob(std::condition_variable& cv, std::mutex& mutex, std::atomic_int& threadsReady, Shader* shader){
        
        while (alive){
    
            std::unique_lock<std::mutex> lm { mutex };
            threadsReady++;
            cv.wait(lm);
            lm.unlock();
            
            if (alive){
                for (int i { 0 }; i < pixelArrSize; i++){
                    shader->DrawPixel(*csi, pixelArr[i].first, pixelArr[i].second, *tex, *fb);
                }
            }
        }
    }
};

Shader::Shader(const glm::mat4& proj, const glm::mat4& view)
    :proj(proj), view(view), THREAD_COUNT(std::thread::hardware_concurrency())
{
    fragmentReadyCount = 0;

    fragmentShaderThreadPool = new FragmentShaderThread[THREAD_COUNT];

    for (int i { 0 }; i < THREAD_COUNT; i++){
    
        fragmentShaderThreadPool[i].thread = std::thread { &FragmentShaderThread::DrawPixelJob, &fragmentShaderThreadPool[i] , std::ref(fragmentStartNotifier), std::ref(fragmentStart), std::ref(fragmentReadyCount), this };
    }

}

Shader::~Shader(){

    for (int i { 0 }; i < THREAD_COUNT; i++){
        fragmentShaderThreadPool[i].alive = false;
    }
    for (int i { 0 }; i < THREAD_COUNT; i++){
        fragmentStartNotifier.notify_all();
    }
    for (int i { 0 }; i < THREAD_COUNT; i++){
        fragmentShaderThreadPool[i].thread.join();
    }

    delete[] fragmentShaderThreadPool;
}

ClipSpaceInfo Shader::ToClipSpace(const Triangle& t, const glm::mat4& model){
    
    ClipSpaceInfo result;
    result.t = t;

    glm::mat4 vm { view * model };
        
    //transform to view space for lighting info
    result.v1view = vm * glm::vec4{t.v1, 1.0f};
    result.v2view = vm * glm::vec4{t.v2, 1.0f};
    result.v3view = vm * glm::vec4{t.v3, 1.0f};

    //get view space normals, TODO: this doesn't work for non linear scaling, it will mess up the normals. look at learnopenGL basic lighting for info
    glm::mat3 normalTransform { vm };

    result.n1view = normalTransform * t.n1;
    result.n2view = normalTransform * t.n2;
    result.n3view = normalTransform * t.n3;

    result.r1view = glm::normalize(glm::reflect(result.v1view - lightInfo.worldPosition, result.n1view));
    result.r2view = glm::normalize(glm::reflect(result.v2view - lightInfo.worldPosition, result.n2view));
    result.r3view = glm::normalize(glm::reflect(result.v3view - lightInfo.worldPosition, result.n3view));
        
    //get in clip space
    glm::vec4 v1, v2, v3;

    v1 = proj * glm::vec4{result.v1view, 1.0f};
    v2 = proj * glm::vec4{result.v2view, 1.0f};
    v3 = proj * glm::vec4{result.v3view, 1.0f};

    glm::mat3 mvpNorms {glm::mat3{proj} * normalTransform};

    result.t.n1 = mvpNorms * t.n1;
    result.t.n2 = mvpNorms * t.n2;
    result.t.n3 = mvpNorms * t.n3;

    if (v1.w == 0) v1.w = 0.000001f;
    if (v2.w == 0) v2.w = 0.000001f;
    if (v3.w == 0) v3.w = 0.000001f;

    result.t.v1 = v1 / v1.w;
    result.t.v2 = v2 / v2.w;
    result.t.v3 = v3 / v3.w;

    //keep z ordering info from clip space
    //result.t.v1.z = v1.z;
    //result.t.v2.z = v2.z;
    //result.t.v3.z = v3.z;

    return result;
}

//TODO: less OOP solution, a general function for just the geometry perhaps? without explicit model struct type
std::vector<ClipSpaceInfo> Shader::ToClipSpace(const Model& m, const glm::mat4& model){

    std::vector<ClipSpaceInfo> result;
    result.reserve( m.triIndexes.size()/2 );
    //2 tris per quad
    for (int i { 0 }; i+3 < m.triIndexes.size(); i+=4){
        
        //create 2 tris
        Triangle t[] {
            {
            m.vertices[static_cast<size_t>(m.triIndexes[i].x) - 1],
            m.vertices[static_cast<size_t>(m.triIndexes[i + 1].x) - 1],
            m.vertices[static_cast<size_t>(m.triIndexes[i + 2].x) - 1],
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            m.texCoords[static_cast<size_t>(m.triIndexes[i].y) - 1],
            m.texCoords[static_cast<size_t>(m.triIndexes[i + 1].y) - 1],
            m.texCoords[static_cast<size_t>(m.triIndexes[i + 2].y) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i].z) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i + 1].z) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i + 2].z) - 1]
            }
            ,
            {
            m.vertices[static_cast<size_t>(m.triIndexes[i].x) - 1],
            m.vertices[static_cast<size_t>(m.triIndexes[i + 2].x) - 1],
            m.vertices[static_cast<size_t>(m.triIndexes[i + 3].x) - 1],
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
            m.texCoords[static_cast<size_t>(m.triIndexes[i].y) - 1],
            m.texCoords[static_cast<size_t>(m.triIndexes[i + 2].y) - 1],
            m.texCoords[static_cast<size_t>(m.triIndexes[i + 3].y) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i].z) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i + 2].z) - 1],
            m.normals[static_cast<size_t>(m.triIndexes[i + 3].z) - 1]
            }
        };

        //transform to clip space
        //add to result vector
        ClipSpaceInfo csi1 { ToClipSpace(t[0], model) };
        ClipSpaceInfo csi2 { ToClipSpace(t[1], model) };
        //TODO: currently this only takes into account 1 normal to dot for backface culling, so it won't work for 3 verts with different normals (idk when that would be...) (maybe this is good)
        //Also this assumes that the quad always has the same normals for both triangles which... feels correct? Idk still a novice so might be wrong
        if (glm::dot(csi1.t.n1, glm::vec3{0.0f, 0.0f, 0.0f} - glm::vec3{(csi1.t.v1 + csi1.t.v2 + csi1.t.v3) / 3.0f}) > 0) {
            result.push_back(csi1);
            result.push_back(csi2);
        }

    }
    return result;
}

void Shader::RasterizeTriangle(ClipSpaceInfo clipInfo, const Texture& tex, FrameBuffer& fb){

    using namespace std;

    

    //get in screen space
    clipInfo.t.v1.x = (clipInfo.t.v1.x + 1) * (fb.width / 2);
    clipInfo.t.v1.y = (clipInfo.t.v1.y + 1) * (fb.height / 2);
    clipInfo.t.v2.x = (clipInfo.t.v2.x + 1) * (fb.width / 2);
    clipInfo.t.v2.y = (clipInfo.t.v2.y + 1) * (fb.height / 2);
    clipInfo.t.v3.x = (clipInfo.t.v3.x + 1) * (fb.width / 2);
    clipInfo.t.v3.y = (clipInfo.t.v3.y + 1) * (fb.height / 2);

    glm::vec3 verts[]{clipInfo.t.v1, clipInfo.t.v2, clipInfo.t.v2, clipInfo.t.v3, clipInfo.t.v3, clipInfo.t.v1};

    //storage for pixels drawn in specific format
    vector<pair<uint16_t,uint16_t>> xLineIndexes;
    //information regarding bounds of the triangle, want the bounding box to also be bounded by the window dimensions as there's no reason to store x or y values outside of the screen
    int xUpper { min(static_cast<int>(max({floor(clipInfo.t.v1.x), floor(clipInfo.t.v2.x), floor(clipInfo.t.v3.x), 0.f})), fb.width - 1) };
    int yUpper { min(static_cast<int>(max({floor(clipInfo.t.v1.y), floor(clipInfo.t.v2.y), floor(clipInfo.t.v3.y), 0.f})), fb.height - 1)};
    int xLower { max(static_cast<int>(min({floor(clipInfo.t.v1.x), floor(clipInfo.t.v2.x), floor(clipInfo.t.v3.x), static_cast<float>(fb.width - 1)})), 0) };
    int yLower { max(static_cast<int>(min({floor(clipInfo.t.v1.y), floor(clipInfo.t.v2.y), floor(clipInfo.t.v3.y), static_cast<float>(fb.height - 1)})), 0) };
    //each vector represents the triangles x points on the lines of the triangle
    xLineIndexes.resize(abs(yUpper - yLower) + 1);
    //ensure that any x value present will be less than the initial minimum x
    for (auto& [xMin, _ ] : xLineIndexes){
        xMin = fb.width - 1;
    }

    std::vector<std::pair<glm::vec2, int>> pixelsToDraw;
    pixelsToDraw.reserve(32);

    //CALC OUTLINES OF TRI
    for (int i {0}; i < 6; i+=2){
        
        glm::vec3 v1 { verts[i] };
        glm::vec3 v2 { verts[i+1] };

        float dx{ v1.x - v2.x };
        float dy{ v1.y - v2.y };
        float step;
            
        if (abs(dx) > abs(dy)) step = abs(dx);
        else step = abs(dy);
                        
        float xInc { (step == 0)? 0 : dx / step };
        float yInc { (step == 0)? 0 : dy / step };

        for (int i{ 0 }; i <= step; i++){

            //TODO: potential floating point/overflow errors causing indexes to be out of bounds
            uint16_t xIndex { static_cast<uint16_t>(floor(v2.x + (xInc * i))) };
            uint16_t yIndex { static_cast<uint16_t>(floor(v2.y + (yInc * i))) };
            int buffIndex { xIndex + yIndex * fb.width };

            int lineValIndex { yIndex - yLower };

            //TODO: replace this with real clipping, as it could be that coordinates are outside screen but the resulting lines/faces are in the screen
            if (yIndex < 0 || yIndex >= fb.height) continue;

            pair<uint16_t,uint16_t>& xLine { xLineIndexes[lineValIndex] };

            //TODO: depth won't work properly for partial triangles, because these depth values are for the offscreen coordinate, not the one I'm replacing it with
            if (xIndex < 0) {
                xLine.first = 0;
                continue;
            }
            else if (xIndex >= fb.width) {
                xLine.second = fb.width-1;
                continue;
            }

            xLine.first = min(xIndex, xLine.first);
            xLine.second = max(xIndex, xLine.second);

            pixelsToDraw.emplace_back(glm::vec2{xIndex, yIndex}, buffIndex);
        }
    }

    //FILL POLYGON
    for (int i { 0 }; i < xLineIndexes.size(); i++){
            
        int buffIndexStart { xLineIndexes[i].first };
        int buffIndexEnd { xLineIndexes[i].second };

        for (int j { buffIndexStart + 1 }; j < buffIndexEnd; j++){
                
            int colourIndex { j + ((i + yLower) * fb.width) };

            pixelsToDraw.emplace_back(glm::vec2{ j, i + yLower }, colourIndex);
        }
    }

    //single threaded
    //for (int i { 0 }; i < pixelsToDraw.size(); i++){
    //    
    //    DrawPixel(clipInfo, pixelsToDraw[i].first, pixelsToDraw[i].second, tex, fb);
    //    
    //    
    //}
    

    //remainder used to balance workload to threads for drawing pixels
    size_t remainder { pixelsToDraw.size() % THREAD_COUNT };

    int offset { 0 };
    for (int i { 0 }; i < THREAD_COUNT; i++){
    
        size_t pixelsThisThread {pixelsToDraw.size() / THREAD_COUNT};

        //Any remainders from dividing the work by thread should be counted for
        if (i < remainder) {
            pixelsThisThread++;
        }

        fragmentShaderThreadPool[i].Setup(fb, tex, clipInfo, pixelsToDraw.data() + offset, static_cast<int>(pixelsThisThread));    

        //update offset so next thread starts from previous end point
        offset += static_cast<int>(pixelsThisThread);
    }


    std::unique_lock<std::mutex> lm(fragmentStart);
    fragmentReadyCount = 0;
    fragmentStartNotifier.notify_all();
    lm.unlock();

    //TODO: keep main thread running while rendering?
    while (fragmentReadyCount < static_cast<int>(THREAD_COUNT))
    {
        
    }
    
}

void Shader::DrawPixel(const ClipSpaceInfo& clipInfo, glm::vec2 pixelPosition, int bufferIndex, const Texture& tex, FrameBuffer& fb) {
        
    glm::vec3 weights { Maths::BarycentricWeights(clipInfo.t.v1, clipInfo.t.v2, clipInfo.t.v3, {pixelPosition, 0.0f}) };

    //todo: this is bandaid, pixels should not be being drawn if they are outside the triangle
    if (weights.x < 0.0f || weights.y < 0.0f || weights.z < 0.0f) return;

    float pixelDepth { clipInfo.t.v1.z * weights.x + clipInfo.t.v2.z * weights.y + clipInfo.t.v3.z * weights.z };

    //write pixel if depth is lower
    if (fb.Depth[bufferIndex] > pixelDepth) {
                    
        fb.Depth[bufferIndex] = pixelDepth;
            
        if (tex.image.size() > 0) {

            glm::vec2 temp[] {
                clipInfo.t.uv1 * weights.x,
                clipInfo.t.uv2 * weights.y,
                clipInfo.t.uv3 * weights.z
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

            glm::vec3 l { glm::vec3{ view * glm::vec4{lightInfo.worldPosition, 1.0f} } - (clipInfo.v1view * weights.x) + (clipInfo.v2view * weights.y) + (clipInfo.v3view * weights.z) };
            glm::vec3 n { (clipInfo.n1view * weights.x) + (clipInfo.n2view * weights.y) + (clipInfo.n3view * weights.z) }; 
            glm::vec3 r { (clipInfo.r1view * weights.x) + (clipInfo.r2view * weights.y) + (clipInfo.r3view * weights.z) }; 
            glm::vec3 v { (-clipInfo.v1view * weights.x) + (-clipInfo.v2view * weights.y) + (-clipInfo.v3view * weights.z) };
            glm::vec3 intensity { Lighting::GetPhongIllumination(.1f, .7f, 0.2f, 2.0f, l, n, r, v, lightInfo.ambientIntensity, lightInfo.diffuseIntensity, lightInfo.specularIntensity) };


            fb.Colours[bufferIndex] = texSamples * glm::vec4{ intensity, 1.0f };
        }
        else {
            //TODO: these weights are calc'd from rounded coordinates, resulting in potentially inaccurate values
            fb.Colours[bufferIndex] = glm::vec4{clipInfo.t.c1 * weights.x + clipInfo.t.c2 * weights.y + clipInfo.t.c3 * weights.z};
        }
            
    }
}
   
void Shader::DrawModel(const Model& model, const glm::mat4& modelMatrix, FrameBuffer& frameBuffer, const Texture& texture){


    std::vector<ClipSpaceInfo> clipSpaceModel { ToClipSpace(model, modelMatrix) };
    std::vector<ClipSpaceInfo> postAxesTestTris;
    postAxesTestTris.reserve(clipSpaceModel.size());

    int offset[] { 0,1,2 };
    float axisValue[] { -1.f, 1.f, -1.f, 1.f, -1.f, 1.f };
    
    for (int i { 0 }; i < clipSpaceModel.size(); i++){
    
        bool failed = false;
        for (int j { 0 }; j < 3; j++){
    
            if (*(&clipSpaceModel[i].t.v1.x + offset[j]) < axisValue[j * 2] &&
                *(&clipSpaceModel[i].t.v2.x + offset[j]) < axisValue[j * 2] &&
                *(&clipSpaceModel[i].t.v3.x + offset[j]) < axisValue[j * 2]) {
                failed = true;
                break;
            }
            if (*(&clipSpaceModel[i].t.v1.x + offset[j]) > axisValue[(j * 2) + 1] &&
                *(&clipSpaceModel[i].t.v2.x + offset[j]) > axisValue[(j * 2) + 1] &&
                *(&clipSpaceModel[i].t.v3.x + offset[j]) > axisValue[(j * 2) + 1]) {
                failed = true;
                break;
            }
        }

        if (failed) continue;

        //TODO: test whether remaining triangles are fully inside the volume, if so they can skip being clipped


        postAxesTestTris.push_back(clipSpaceModel[i]);
    }

    
    std::vector<ClipSpaceInfo> clippedModel;
    clippedModel.reserve(postAxesTestTris.size());

    for (int i { 0 }; i < postAxesTestTris.size(); i++){
            
        std::vector<ClipSpaceInfo> clippedTris { ClipTriangleOpenGLCanonical(postAxesTestTris[i]) };
        
        for (const ClipSpaceInfo& csi : clippedTris){
            clippedModel.push_back(csi);
        }
            
    }
    for (int i { 0 }; i < clippedModel.size(); i++){
            
        RasterizeTriangle(clippedModel[i], texture, frameBuffer);
            
    }

    std::cout << "Model tri count: " << clipSpaceModel.size() << ". post axes test: " << postAxesTestTris.size() << ". post clipping: " << clippedModel.size() << ". \n";
}

std::vector<ClipSpaceInfo> Shader::ClipTriangleOpenGLCanonical(const ClipSpaceInfo& t){

    std::vector<ClipSpaceInfo> newTris;

    std::vector<glm::vec3> outputList;
    std::vector<glm::vec3> inputList {t.t.v1, t.t.v2, t.t.v3};

    
    //Calculate intersections and create new polygon
    for (int i { 0 }; i < 4; i++){
    
        for (int j { 0 }; j < inputList.size(); j++){
    
            glm::vec3 currentPoint { inputList[j] };
            size_t prevIndex { (j == 0) ? inputList.size()-1 : j - 1};
            glm::vec3 previousPoint { inputList[prevIndex] };

            //TODO: div by 0?
            float m = (currentPoint.y - previousPoint.y) / (currentPoint.x - previousPoint.x);
            float c = currentPoint.y - (m * currentPoint.x);
            switch (i) {
            
            case 0:
                //clip in line x = -1
                if ((currentPoint.x < -1.0f && previousPoint.x > -1.0f) ||
                    (currentPoint.x > -1.0f && previousPoint.x < -1.0f)) {
                    
                    //TODO: lerp z coord to store in vec3
                    glm::vec2 intersection { -1.0f, (m * -1.0f) + c };

                    if (currentPoint.x < -1.0f) outputList.emplace_back(intersection, 1.0f);
                    else {
                        outputList.emplace_back(intersection, 1.0f);
                        outputList.emplace_back(currentPoint);
                    }
                }
                else outputList.emplace_back(currentPoint);
                break;
            case 1:
                //clip in line x = 1
                if ((currentPoint.x > 1.0f && previousPoint.x < 1.0f) ||
                    (currentPoint.x < 1.0f && previousPoint.x > 1.0f)) {
                    
                    //TODO: lerp z coord to store in vec3
                    glm::vec2 intersection { 1.0f, (m * 1.0f) + c };

                    if (currentPoint.x > 1.0f) outputList.emplace_back(intersection, 1.0f);
                    else {
                        outputList.emplace_back(intersection, 1.0f);
                        outputList.emplace_back(currentPoint);
                    }
                }
                else outputList.emplace_back(currentPoint);

                break;
            case 2:
                //clip in line y = -1
                if ((currentPoint.y < -1.0f && previousPoint.y > -1.0f) ||
                    (currentPoint.y > -1.0f && previousPoint.y < -1.0f)) {
                    
                    //TODO: lerp z coord to store in vec3
                    glm::vec2 intersection { (-1.0f - c) / m, -1.0f};

                    if (currentPoint.y < -1.0f) outputList.emplace_back(intersection, 1.0f);
                    else {
                        outputList.emplace_back(intersection, 1.0f);
                        outputList.emplace_back(currentPoint);
                    }
                }
                else outputList.emplace_back(currentPoint);

                break;
            case 3:
                //clip in line y = 1
                if ((currentPoint.y > 1.0f && previousPoint.y < 1.0f) ||
                    (currentPoint.y < 1.0f && previousPoint.y > 1.0f)) {
                    
                    //TODO: lerp z coord to store in vec3
                    glm::vec2 intersection { (1.0f - c) / m, 1.0f};

                    if (currentPoint.y > 1.0f) outputList.emplace_back(intersection, 1.0f);
                    else {
                        outputList.emplace_back(intersection, 1.0f);
                        outputList.emplace_back(currentPoint);
                    }
                }
                else outputList.emplace_back(currentPoint);

                break;
            }

        }
        
        inputList = outputList;
        outputList.clear();
    }


    newTris.reserve(inputList.size()-2);
    //now convert this polygon into triangles
    for (int i { 0 }; i < inputList.size()-2; i++){
        
        //TODO: modify all other values in the triangle like texcoords based on their intersections
        ClipSpaceInfo csi { t };
        csi.t.v1 = inputList[0];
        csi.t.v2 = inputList[i + 1];
        csi.t.v3 = inputList[i + 2];

        newTris.push_back(csi);
    }
    return newTris;
}
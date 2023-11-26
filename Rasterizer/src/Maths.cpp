#include "Maths.h"



namespace Maths {

    //Linear interpolation
    float Lerp(float x1, float x2, float ratio) {
        float range = x2 - x1;
        return x1 + (range * ratio);
    }

    //Fractional Part
    float Fractional(float x){
        x = abs(x);
        return x - static_cast<int>(x);
    }

    //barycentric weights TODO: vec2 overload, also why tf does marking this inline give me linker errors
    glm::vec3 BarycentricWeights(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& p){

        float det { ((v2.y - v3.y)*(v1.x - v3.x) + (v3.x - v2.x)*(v1.y - v3.y)) };
        //TODO: dunno if any other fix better
        if (det == 0.0f) det = 0.00000000001f;

        float w1 { ((v2.y - v3.y)*(p.x - v3.x) + (v3.x - v2.x)*(p.y - v3.y)) / det };
        float w2 { ((v3.y - v1.y)*(p.x - v3.x) + (v1.x - v3.x)*(p.y - v3.y)) / det };
        float w3 { 1 - w1 - w2 };

        //barycentric weights must add up to essentially 1
        //TODO: When looking at a tri side on, it's screenspace coords could have x and/or y being the same for all points, seems to cause problems.
        return { w1, w2, w3};
    }
}
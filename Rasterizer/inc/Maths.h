#pragma once
#include "glm.hpp"

namespace Maths {

	//Linear interpolation
	float Lerp(float x1, float x2, float ratio);
	//Returns fractional part of a float
	 float Fractional(float x);
	//barycentric weights TODO: vec2 overload
	glm::vec3 BarycentricWeights(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& p);
}
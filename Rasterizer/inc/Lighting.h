#pragma once
#include "glm.hpp"

namespace Lighting{

	glm::vec3 GetPhongIllumination(float ka, float kd, float ks, float shininess, glm::vec3 l, glm::vec3 n, glm::vec3 r, glm::vec3 v);








};
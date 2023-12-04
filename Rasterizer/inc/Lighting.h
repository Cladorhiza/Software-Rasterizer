#pragma once
#include "glm.hpp"

namespace Lighting{

	glm::vec3 GetPhongIllumination(const glm::vec3& ka, const glm::vec3& kd, const glm::vec3& ks, float shininess, 
								   glm::vec3 l, glm::vec3 n, glm::vec3 r, glm::vec3 v,
								   const glm::vec3& aI, const glm::vec3& dI, const glm::vec3& sI);








};
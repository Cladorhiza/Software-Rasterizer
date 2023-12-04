#include "Lighting.h"


namespace Lighting{

	glm::vec3 GetPhongIllumination(const glm::vec3& ka, const glm::vec3& kd, const glm::vec3& ks, float shininess, 
								   glm::vec3 l, glm::vec3 n, glm::vec3 r, glm::vec3 v,
								   const glm::vec3& aI, const glm::vec3& dI, const glm::vec3& sI){
		
		l = glm::normalize(l);
		r = glm::normalize(r);
		v = glm::normalize(v);

		glm::vec3 ambientTerm { aI * ka };
		glm::vec3 diffuseTerm { 0.0f, 0.0f, 0.0f };
		glm::vec3 specularTerm { 0.0f, 0.0f, 0.0f };
		
		float nDotL { glm::dot(n,l) };
		//Only calc specular highlights if surface is lit by diffuse light
		if (nDotL > 0.0f){
			diffuseTerm = dI * kd * nDotL;
			glm::vec3 specularTerm { sI * ks * pow(glm::dot(r, v), shininess)};
		}

		return ambientTerm + diffuseTerm + specularTerm;
	}
};
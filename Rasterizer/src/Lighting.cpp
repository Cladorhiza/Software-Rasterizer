#include "Lighting.h"


namespace Lighting{

	glm::vec3 GetPhongIllumination(float ka, float kd, float ks, float shininess, 
								   glm::vec3 l, glm::vec3 n, glm::vec3 r, glm::vec3 v,
								   const glm::vec4& aI, const glm::vec4& dI, const glm::vec4& sI){
		
		

		glm::vec3 ambientTerm { aI * ka };
		
		
		l = glm::normalize(l);
		glm::vec3 diffuseTerm;
		float nDotL { glm::dot(n,l) };
		if (nDotL > 0.0f){
			diffuseTerm = dI * kd * nDotL;
		}
		else diffuseTerm = {0.0f, 0.0f, 0.0f};

		r = glm::normalize(r);
		v = glm::normalize(v);
		glm::vec3 specularTerm { sI * ks * pow(glm::dot(r, v), shininess)};
		
		glm::vec3 illuminance { ambientTerm + diffuseTerm + specularTerm };

		return illuminance;
	}
};
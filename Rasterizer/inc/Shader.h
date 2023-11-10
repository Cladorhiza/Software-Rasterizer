#pragma once

#include <string>
#include <unordered_map>

#include "glm.hpp"

struct ShaderProgramSource {

	std::string vertexSource;
	std::string fragmentSource;
	std::string geometrySource;

};

class Shader {
private:
	std::string filePath;

	//OpenGL Shader ID
	unsigned int rendererID;

	//Maps names to uniform locations, used to speed up SetUniforms
	std::unordered_map<std::string, int> m_UniformLocationCache;
public:
	Shader(const std::string& filepath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	//Set uniforms
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);
	void SetUniform1iv(const std::string& name, const int size, const int* const data);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformvec3f(const std::string& name, float v0, float v1, float v2);
	void SetUniform1i(const std::string& name, int vi);
	void SetUniform1f(const std::string& name, float f);


	//Helpers which setup shaders from source to openGL object
	static ShaderProgramSource ParseShader(const std::string& filepath);
	static unsigned int CompileShader(unsigned int type, const std::string& source);
	static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader, const std::string& geometryShader);
private:
	int GetUniformLocation(const std::string& name);
};
#pragma once

#include <vector>
#include <string>

namespace FileParsing{

	struct OBJFileParseResult{

		std::vector<float> vertices;
		std::vector<float> texCoords;
		std::vector<float> normals;
		std::vector<unsigned> indexes;
	};

	OBJFileParseResult ParseObjToData(const std::vector<std::string> fileText);

	std::vector<std::string> LoadFile(std::string filePath);

	std::vector<std::string> SplitString(const std::string& s, char delim);
}
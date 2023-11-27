#include "FileParsing.h"

#include <fstream>
#include <iostream>

namespace FileParsing{

	using namespace std;

	OBJFileParseResult ParseObjToData(const vector<string> fileText){

		OBJFileParseResult result;

		size_t lineCount { fileText.size() };

		//the obj will have roughly equal numbers of elements bar indexes
		result.vertices.reserve(lineCount / 3);
		result.texCoords.reserve(lineCount / 3);
		result.normals.reserve(lineCount / 3);
		result.indexes.reserve(lineCount / 6);

		for (const string& s : fileText){
			
			if (s.size() == 0) continue; //ignore empty lines
			if (s[0] == '#') continue; //ignore comments

			vector<string> split { SplitString(s, ' ') };

			if (split[0] == "v") {
				result.vertices.emplace_back(stof(split[1]));
				result.vertices.emplace_back(stof(split[2]));
				result.vertices.emplace_back(stof(split[3]));
			}
			else if (split[0] == "vt") {
				result.texCoords.emplace_back(stof(split[1]));
				result.texCoords.emplace_back(stof(split[2]));
			}
			else if (split[0] == "vn") {
				result.normals.emplace_back(stof(split[1]));
				result.normals.emplace_back(stof(split[2]));
				result.normals.emplace_back(stof(split[3]));
			}
			else if (split[0] == "f") {
				for (int i {1}; i < 5; i++){
					vector<string> miniSplit { SplitString(split[i], '/') };
					result.indexes.emplace_back(stoi(miniSplit[0]));
					result.indexes.emplace_back(stoi(miniSplit[1]));
					result.indexes.emplace_back(stoi(miniSplit[2]));
				}
			}
		}
		return result;
	}

	vector<string> LoadFile(string filePath){

		vector<string> result;
		string line;
		ifstream file{ filePath };
		
		if (!file.is_open()){
			cout << "Failed to open file: " << filePath << "! :C\n";
			return result;
		}
		
		while (getline(file, line)) {
			result.push_back(line);
		}
		
		return result;
	}

	vector<string> SplitString(const string& s, char delim){
		
		size_t start = 0;
		vector<string> tokens;
		size_t end { s.find(delim, start) };
		while (end != s.npos){

			tokens.emplace_back(s.substr(start, end-start));
			//move start to after delim
			start = end + 1;
			end = s.find(delim, start);
		}

		//if no splits possible, return whole string
		if (tokens.size() == 0) {
			tokens.emplace_back(s);	
		}
		//if size is greater than 0, it means there's a final section of string after the last split to append
		else {
			tokens.emplace_back(s.substr(start, s.size() - start));
		}

		return tokens;
	}
}
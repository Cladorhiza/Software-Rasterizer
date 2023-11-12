#pragma once
#include <glfw3.h>
#include <unordered_map>


namespace InputManager
{
	void Init(GLFWwindow* window);
	void Poll(GLFWwindow* window);
	int GetKeyState(const int GLFWKeyCode);
	bool GetKeyToggle(int GLFWKeyCode);
	double GetMouseX();
	double GetMouseY();
	double GetMouseScrollYOffset();
	double GetDeltaMouseX();
	double GetDeltaMouseY();
};

#include "InputManager.h"
#include <unordered_map>

namespace InputManager{

	//all keycodes that need to be tracked
	std::vector<int> usedKeys{
		GLFW_KEY_W,
		GLFW_KEY_A,
		GLFW_KEY_S,
		GLFW_KEY_D,
		GLFW_KEY_L,
		GLFW_KEY_K,
		GLFW_KEY_C,
		GLFW_KEY_X,
		GLFW_KEY_Q,
		GLFW_KEY_E,
		GLFW_KEY_ESCAPE,
		GLFW_KEY_F2
	};
	//stored state of keys
	std::unordered_map<int, int> keys;
	//has key toggled this frame
	std::unordered_map<int, bool> keyToggle;
	//mouse position
	double cursorXPos = 0, cursorYPos = 0, cursorOldXPos = 0, cursorOldYPos = 0;
	double scrollYAxisOffset = 0;

	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		scrollYAxisOffset += yoffset;
	}
	void Init(GLFWwindow* window) {
		glfwSetScrollCallback(window, ScrollCallback);
	}

	void Poll(GLFWwindow* window) {
		//updates glfw keys
		glfwPollEvents();

		//updates state of keys
		
		for (int keyName : usedKeys) {
			int state = glfwGetKey(window, keyName);
			if (state != keys[keyName] && keys[keyName] == GLFW_RELEASE) keyToggle[keyName] = true;
			else keyToggle[keyName] = false;
			keys[keyName] = state;
		}
		keys[GLFW_MOUSE_BUTTON_LEFT] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		keys[GLFW_MOUSE_BUTTON_RIGHT] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

		//update cursor position
		cursorOldXPos = cursorXPos;
		cursorOldYPos = cursorYPos;
		glfwGetCursorPos(window, &cursorXPos, &cursorYPos);
		
	}
	int GetKeyState(const int GLFWKeyCode) {
		return keys[GLFWKeyCode];
	}
	bool GetKeyToggle(int GLFWKeyCode) {
		return keyToggle[GLFWKeyCode];
	}
	double GetMouseX() { return cursorXPos; }
	double GetMouseY() { return cursorYPos; }
	double GetMouseScrollYOffset() { return scrollYAxisOffset; }
	double GetDeltaMouseX() { return (cursorXPos - cursorOldXPos); }
	double GetDeltaMouseY() { return (cursorYPos - cursorOldYPos); }

}

// LearnVK.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef LEARN_VK_APP
#define LEARN_VK_APP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

class LearnVKApp
{
public:
	void run();

private:
	void initVK();

	void createVKInstance();

	void setupDebugCallback();

	std::vector<const char*> getRequiredExtentions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messagesSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messgaesType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);

	bool checkExtentionsProperties(std::vector<const char*>& extentionName);

	bool checkValidationLayersProperties();

	void initWindows();

	void loop();

	void clear();

	GLFWwindow* m_window = nullptr;

	VkInstance m_vkInstance;

	// 存储创建的回调函数信息
	VkDebugUtilsMessengerEXT m_callBack;

	std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };

	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif	
};

VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback);

void destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT* pCallback,
	const VkAllocationCallbacks* pAllocator
);
#endif
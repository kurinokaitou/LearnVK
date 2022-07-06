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
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct QueueFamiliyIndices
{
	std::set<int> familiesIndexSet;
	int graphicsFamily = -1;
	int presentFamily = -1;
	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
	bool isSameFamily() {
		return graphicsFamily == presentFamily;
	}
	int familyCount() {
		return static_cast<int>(familiesIndexSet.size());
	}
};

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

	bool checkInstanceExtentionsSupport(std::vector<const char*>& extentionName);

	bool checkDeviceExtentionsSupport(VkPhysicalDevice physicalDevice);

	bool checkValidationLayersProperties();

	QueueFamiliyIndices findDeviceQueueFamilies(VkPhysicalDevice physicalDevice);

	bool isDeviceSuitable(VkPhysicalDevice device);
	
	void createSurface();
	
	void pickPhysicalDevice();

	void createLogicalDevice();

	void initWindows();

	void loop();

	void clear();

	GLFWwindow* m_window = nullptr;

	VkSurfaceKHR m_surface;

	VkInstance m_vkInstance;

	// 存储创建的回调函数信息
	VkDebugUtilsMessengerEXT m_callBack;

	// 支持一台物理设备
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	// 一台逻辑设备
	VkDevice m_device = VK_NULL_HANDLE;

	std::map<std::string, VkQueue> m_queueMap;

	std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_deviceExtentions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
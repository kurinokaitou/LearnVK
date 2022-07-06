// LearnVK.cpp: 定义应用程序的入口点。
//
#include "LearnVKApp.h"

void LearnVKApp::run() {	// 开始运行程序
	initWindows();
	initVK();
	loop();
	clear();
}

void LearnVKApp::initWindows() {	// 初始化glfw
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LearnVK", nullptr, nullptr);
	
}

void LearnVKApp::initVK() {	// 初始化Vulkan的设备
	createVKInstance();
	setupDebugCallback();
}

void LearnVKApp::createVKInstance() {
	// 检查校验层
	if (enableValidationLayers && !checkValidationLayersProperties()) {
		throw std::runtime_error("failed to use validation layers!");
	}

	// 填写应用创建信息
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "LearnVKApp";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// 填写Vulkan实例创建信息
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	// 填写glfw窗口系统扩展的信息
	auto extentionsName = getRequiredExtentions();

	createInfo.enabledExtensionCount =  static_cast<uint32_t>(extentionsName.size());
	createInfo.ppEnabledExtensionNames = extentionsName.data();
	// 填写校验层信息
	if (enableValidationLayers) {
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	

	// 在实例创建之前检查实例支持的扩展列表
	if (!checkExtentionsProperties(extentionsName)) {
		throw std::runtime_error("failed to support glfw instance extention");
	}

	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create vk instance");
	}
}

// 设置debug的回调函数
void LearnVKApp::setupDebugCallback() {
	if (!enableValidationLayers) return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
	VkResult res = createDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_callBack);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to setup debug callback!");
	}
}

// DebugUtilsMessage 被Vulkan调用的静态用户定义行为函数，在一般情况只返回VK_FALSE
VKAPI_ATTR VkBool32 VKAPI_CALL LearnVKApp::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messagesSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messgaesType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	std::cerr << "validation error:" << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

// 获取glfw和校验层的扩展
std::vector<const char*> LearnVKApp::getRequiredExtentions() {
	uint32_t glfwExtentionCount = 0;
	const char** glfwExtentionName;
	glfwExtentionName = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);
	std::vector<const char*> extentionList(glfwExtentionName, glfwExtentionName + glfwExtentionCount);
	if (enableValidationLayers) {
		extentionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extentionList;
}

// 只检查glfw的扩展，因为校验层扩展已经通过检查
bool LearnVKApp::checkExtentionsProperties(std::vector<const char*>& extentionsName) {
	uint32_t extentionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, nullptr);
	std::vector<VkExtensionProperties> extentionsProperties(extentionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, extentionsProperties.data());
	std::cout << "\t" << extentionsProperties.size() << std::endl;
	for (auto& properties : extentionsProperties) {
		std::cout << "\t" << properties.extensionName << std::endl;
	}
	for (const char* name : extentionsName) {
		if (std::find_if(extentionsProperties.begin(), extentionsProperties.end(), 
			[name](VkExtensionProperties val) {
				return std::strcmp(val.extensionName, name) == 0;
			}) == extentionsProperties.end()) {
			return false;
		} 
	}
	return true;
}

bool LearnVKApp::checkValidationLayersProperties() {
	uint32_t validationLayersCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayersCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(validationLayersCount);
	vkEnumerateInstanceLayerProperties(&validationLayersCount, availableLayers.data());
	// 检查所有m_validationLayers中要求的layer层是否都支持
	for (auto& layer : m_validationLayers) {
		if (std::find_if(availableLayers.begin(), availableLayers.end(), [layer](VkLayerProperties properties) {
			return std::strcmp(properties.layerName, layer) == 0;
			}) == availableLayers.end()) {
			return false;
		}
	}
	return true;
}

void LearnVKApp::loop() {	// 应用的主循环
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

void LearnVKApp::clear() {	// 释放Vulkan的资源
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}



VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}



int main()
{
	LearnVKApp app;
	try {
		app.run();
	} catch (const std::exception& e) {		// Vulkan主循环中出现的异常在这里被捕获
		std::cerr << e.what() << std::endl;
	}
}

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
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
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
	// 填写glfw窗口系统扩展和debug messager扩展的信息
	auto extentionsName = getRequiredExtentions();

	createInfo.enabledExtensionCount =  static_cast<uint32_t>(extentionsName.size());
	createInfo.ppEnabledExtensionNames = extentionsName.data();
	// 填写校验层信息
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	

	// 在实例创建之前检查实例支持的扩展列表, 只检查glfw的扩展，因为校验层扩展已经通过检查
	if (!checkInstanceExtentionsSupport(extentionsName)) {
		throw std::runtime_error("failed to support instance extention");
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

void LearnVKApp::createSurface() {
	VkResult res = glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
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

bool LearnVKApp::checkInstanceExtentionsSupport(std::vector<const char*>& extentionsName) {
	uint32_t extentionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, nullptr);
	assert(extentionCount != 0);
	std::vector<VkExtensionProperties> extentionsProperties(extentionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, extentionsProperties.data());
	//打印可用的扩展信息
	/*std::cout << "\t" << extentionsProperties.size() << std::endl;
	for (auto& properties : extentionsProperties) {
		std::cout << "\t" << properties.extensionName << std::endl;
	}*/
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

bool LearnVKApp::checkDeviceExtentionsSupport(VkPhysicalDevice physicalDevice) {
	uint32_t extentionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice ,nullptr, &extentionCount, nullptr);
	assert(extentionCount != 0);
	std::vector<VkExtensionProperties> extentionsProperties(extentionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice ,nullptr, &extentionCount, extentionsProperties.data());
	//打印可用的扩展信息
	/*std::cout << "\t" << extentionsProperties.size() << std::endl;
	for (auto& properties : extentionsProperties) {
		std::cout << "\t" << properties.extensionName << std::endl;
	}*/
	for (const char* name : m_deviceExtentions) {
		if (std::find_if(extentionsProperties.begin(), extentionsProperties.end(),
			[name](VkExtensionProperties val) {
				return std::strcmp(val.extensionName, name) == 0;
			}) == extentionsProperties.end()) {
			return false;
		}
	}
	return true;
}

SwapChainSupportDetails LearnVKApp::queryDeviceSwapChainSupport(VkPhysicalDevice physicalDevice) {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &details.surfaceCapabilities);
	// 获取物理设备表面支持的格式
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, details.formats.data());
	}
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, details.presentModes.data());
	}
	return details;
}

void LearnVKApp::createSwapChain() {
	SwapChainSupportDetails details = queryDeviceSwapChainSupport(m_physicalDevice);
	VkSurfaceFormatKHR surfaceFormat = chooseBestfitSurfaceFormat(details.formats);
	VkPresentModeKHR presentMode = chooseBestfitPresentMode(details.presentModes);
	VkExtent2D extent = chooseSwapExtent(details.surfaceCapabilities);
	uint32_t imageCount = details.surfaceCapabilities.minImageCount + 1;
	uint32_t maxImageCount = details.surfaceCapabilities.maxImageCount;
	if (maxImageCount > 0 && imageCount > maxImageCount) {
		imageCount = maxImageCount;
	} 
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageArrayLayers = 1;
	// 只是绘制选择颜色附着，如果需要后处理则需要选择TRANSFER_DST
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageExtent = extent;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;

	QueueFamiliyIndices queueFamilyIndices = findDeviceQueueFamilies(m_physicalDevice);
	auto familyIndicesSet = queueFamilyIndices.familiesIndexSet;
	if (familyIndicesSet.size() != 1) {	// 存在多个队列族就会出现并发申请图片资源的问题
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<uint32_t>(familyIndicesSet.size());
		createInfo.pQueueFamilyIndices = std::vector<uint32_t>(familyIndicesSet.begin(), familyIndicesSet.end()).data();
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;		// 如果只有一个就不存在并发问题，这里随意填写即可
		createInfo.pQueueFamilyIndices = nullptr;
	}
	createInfo.preTransform = details.surfaceCapabilities.currentTransform;	// 预设的缩放旋转等
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// 窗口透明混合策略
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;	// 允许被遮挡像素剔除
	createInfo.oldSwapchain = VK_NULL_HANDLE;	// 重建交换链时用于指定之前的交换链

	VkResult res = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}
	uint32_t imageCount = 0;
	// 获取交换链图像的句柄
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
}

bool LearnVKApp::checkValidationLayersProperties() {
	uint32_t validationLayersCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayersCount, nullptr);
	assert(validationLayersCount != 0);
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

void LearnVKApp::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to pick a GPU with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physicalDevices.data());
	for (auto& device : physicalDevices) {
		if (isDeviceSuitable(device)) {
			m_physicalDevice = device;
			break;
		}
	}
	if (m_physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to pick a suitable GPU!");
	}
}

// 通过获取properties 和 features然后判断是否满足需求
bool LearnVKApp::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
	// TODO: judge device is suitable
	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceFeatures features = {};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);

	// 根据设备获取队列族，如果存在满足VK_QUEUE_GRAPHICS_BIT的队列族即可
	QueueFamiliyIndices indices = findDeviceQueueFamilies(physicalDevice);
	// 查询设备是否支持要求的扩展
	bool extentionsSupport = checkDeviceExtentionsSupport(physicalDevice);
	bool swapChainAdequate = false;
	if (extentionsSupport) {
		auto swapChainDetails = queryDeviceSwapChainSupport(physicalDevice);
		swapChainAdequate = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
	}
	return indices.isComplete() && extentionsSupport && swapChainAdequate;
}

void LearnVKApp::createLogicalDevice() {
	QueueFamiliyIndices indices = findDeviceQueueFamilies(m_physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.0f;
	for (auto index : indices.familiesIndexSet) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	// TODO: 填写物理设备features
	VkPhysicalDeviceFeatures features = {};

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	// 只创建一个队列
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &features;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtentions.size());
	deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtentions.data();
	// 校验层
	if (enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
	}
	// 创建逻辑设备
	VkResult res = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}
	// 命令队列在创建逻辑设备时被一起创建，因为我们只创建了一个队列，所以直接获取索引0的队列即可
	VkQueue queue;
	vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.graphicsFamily), 0, &queue);	// 获取到队列句柄,并添加到map中去
	m_queueMap.insert(std::make_pair("graphicsFamily", queue));
	vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.presentFamily), 0, &queue);
	m_queueMap.insert(std::make_pair("presentFamily", queue));
}

QueueFamiliyIndices LearnVKApp::findDeviceQueueFamilies(VkPhysicalDevice physicalDevice) {
	QueueFamiliyIndices indices;
	uint32_t deviceQueueFamilyCount = 0;
	VkBool32 presentSupport = false;

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceQueueFamilyCount, nullptr);
	assert(deviceQueueFamilyCount != 0);
	std::vector<VkQueueFamilyProperties> properties(deviceQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceQueueFamilyCount, properties.data());

	for (int i = 0; i < deviceQueueFamilyCount; i++) {
		VkQueueFamilyProperties queueFamily = properties[i];
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
				indices.graphicsFamily = i;
				indices.presentFamily = i;
				break;
			}
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			if (presentSupport) {
				indices.presentFamily = i;
			}
		}
	}
	indices.familiesIndexSet.insert(indices.graphicsFamily);
	indices.familiesIndexSet.insert(indices.presentFamily);
	return indices;
}


void LearnVKApp::loop() {	// 应用的主循环
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

void LearnVKApp::clear() {	// 释放Vulkan的资源
	if (enableValidationLayers) {
		destroyDebugUtilsMessengerEXT(m_vkInstance, &m_callBack, nullptr);
	}
	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

VkExtent2D LearnVKApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = {WINDOW_WIDTH, WINDOW_HEIGHT};
		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

// 选择一个最好的格式，如果有SRGB就选SRGB
VkSurfaceFormatKHR chooseBestfitSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return {
			VK_FORMAT_B8G8R8A8_SNORM,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
	} else {
		for (auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return format;
			}
		}
	}
	return formats[0];
}

// 选择一个最好的显示模式，有三缓冲会直接选择三缓冲
VkPresentModeKHR chooseBestfitPresentMode(std::vector<VkPresentModeKHR>& presentModes) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	for (auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		} else if(presentMode == VK_PRESENT_MODE_FIFO_KHR) {
			bestMode = VK_PRESENT_MODE_FIFO_KHR;
		}
	}
	return bestMode;
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

void destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT* pCallback,
	const VkAllocationCallbacks* pAllocator
) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, *pCallback, pAllocator);
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

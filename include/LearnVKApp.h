// LearnVK.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef LEARN_VK_APP
#define LEARN_VK_APP
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vertex_vert.h>
#include <fragment_frag.h>

const static uint64_t MAX_TIMEOUT = std::numeric_limits<uint64_t>::max();
const static int MAX_FRAMES_IN_FLIGHT = 2;	// 代表预渲染队列的帧数量， 如果为1则无预渲染

struct QueueFamiliyIndices
{
	std::set<uint32_t> familiesIndexSet;
	int graphicsFamily = -1;
	int presentFamily = -1;
	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0;
	}
	bool isSameFamily() {
		return graphicsFamily == presentFamily;
	}
	int familyCount() {
		return static_cast<uint32_t>(familiesIndexSet.size());
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
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

	static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

	bool checkInstanceExtentionsSupport(std::vector<const char*>& extentionName);

	bool checkDeviceExtentionsSupport(VkPhysicalDevice physicalDevice);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	SwapChainSupportDetails queryDeviceSwapChainSupport(VkPhysicalDevice physicalDevice);

	bool checkValidationLayersProperties();

	QueueFamiliyIndices findDeviceQueueFamilies(VkPhysicalDevice physicalDevice);

	bool isDeviceSuitable(VkPhysicalDevice device);
	
	void createSurface();
	
	void pickPhysicalDevice();

	void createLogicalDevice();

	void createSwapChain();

	void createImageViews();

	void createRenderPass();

	void createGraphicsPipeline();

	void createFrameBuffers();

	void createCommandPool();

	void createCommandBuffers();

	void createSyncObjects();

	void recordCommandBuffers(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	VkShaderModule createShaderModule(const std::vector<unsigned char>& code);

	void initWindows();

	void loop();

	void drawFrame();

	void cleanupSwapChain();

	void recreateSwapChain();

	void clear();

	GLFWwindow* m_window = nullptr;

	static bool s_framebufferResized;

	VkSurfaceKHR m_surface;

	VkInstance m_vkInstance;

	// 存储创建的回调函数信息
	VkDebugUtilsMessengerEXT m_callBack;

	// 支持一台物理设备
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

	// 一台逻辑设备
	VkDevice m_device = VK_NULL_HANDLE;

	// 交换链
	VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

	// 交换链中的图像句柄，我们操作其来渲染
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainImageExtent;

	// 对图像进行操作的views
	std::vector<VkImageView> m_swapChainImageViews;
	// 缓冲区
	std::vector<VkFramebuffer> m_swapChainFrameBuffers;

	// 管线
	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;

	// 队列族对应的指令队列
	std::map<std::string, VkQueue> m_queueMap;

	// 指令池
	VkCommandPool m_commandPool;
	// 指令缓冲
	std::vector<VkCommandBuffer> m_commandBuffers;
	// 信号量
	std::vector<VkSemaphore> m_imageAvailableSemaphore;
	std::vector<VkSemaphore> m_renderFinishSemaphore;
	// 栅栏
	std::vector<VkFence> m_fences;
	int m_currentFrameIndex = 0;

	std::vector<const char*> m_validationLayers{ "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> m_deviceExtentions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif	
};

static VkSurfaceFormatKHR chooseBestfitSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats);

static VkPresentModeKHR chooseBestfitPresentMode(std::vector<VkPresentModeKHR>& presentModes);

static VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback);

static void destroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT* pCallback,
	const VkAllocationCallbacks* pAllocator
);

static std::vector<char> readFile(const std::string& filename);
#endif
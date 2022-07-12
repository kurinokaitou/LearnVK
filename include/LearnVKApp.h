// LearnVK.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef LEARN_VK_APP
#define LEARN_VK_APP
#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define GLM_ENABLE_EXPERIMENTAL
//#define PRINT_EXTENTION_INFO

#include "tiny_obj_loader.h"
#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <vendor/stb_image.h>
#include <vulkan/vulkan.h>

#include <fragment_frag.h>
#include <vertex_vert.h>

const static uint64_t MAX_TIMEOUT = std::numeric_limits<uint64_t>::max();
const static int MAX_FRAMES_IN_FLIGHT =
    2; // 代表预渲染队列的帧数量， 如果为1则无预渲染
const static std::string CURRENT_PATH =
    std::filesystem::current_path().generic_string();
const static std::string RESOURCE_PATH = std::filesystem::current_path()
                                             .parent_path()
                                             .generic_string()
                                         + "/resource/";
const static std::string TEXTURE_PATH = RESOURCE_PATH + "textures/";
const static std::string MODEL_PATH = RESOURCE_PATH + "models/";

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindDescription() {
        VkVertexInputBindingDescription bindDescription = {};
        bindDescription.binding = 0;
        bindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindDescription.stride = sizeof(Vertex);
        return bindDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescription{};
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].offset = offsetof(Vertex, position);
        attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].offset = offsetof(Vertex, color);
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[2].binding = 0;
        attributeDescription[2].location = 2;
        attributeDescription[2].offset = offsetof(Vertex, texCoord);
        attributeDescription[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        return attributeDescription;
    }
    bool operator==(const Vertex& other) const {
        return position == other.position && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
} // namespace std

struct QueueFamiliyIndices {
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

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class LearnVKApp {
public:
    void run();

private:
    void initVK();

    void createVKInstance();

    void setupDebugCallback();

    std::vector<const char*> getRequiredExtentions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messagesSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messgaesType,
                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                  void* pUserData);

    static void frameBufferResizeCallback(GLFWwindow* window, int width,
                                          int height);

    bool checkInstanceExtentionsSupport(std::vector<const char*>& extentionName);

    bool checkDeviceExtentionsSupport(VkPhysicalDevice physicalDevice);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    SwapChainSupportDetails
    queryDeviceSwapChainSupport(VkPhysicalDevice physicalDevice);

    bool checkValidationLayersProperties();

    QueueFamiliyIndices findDeviceQueueFamilies(VkPhysicalDevice physicalDevice);

    bool isDeviceSuitable(VkPhysicalDevice device);

    void createSurface();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFrameBuffers();

    void createDepthResources();

    void createColorResources();

    VkFormat findDepthFormat();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& formats,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    void createCommandPool();

    void createTextureImage(const std::string& textureName);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usags,
                     VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& memory);

    void generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

    void transitionImageLayout(VkImage image, VkFormat format,
                               VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipsLevels);

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                           uint32_t height);

    void createTextureImageView();

    void createTextureSampler();

    VkImageView createImageView(VkImage image, VkFormat format,
                                VkImageAspectFlags aspectMask, uint32_t mipLevels);

    void loadModel(const std::string& modelName);

    template <typename T>
    void createLocalBuffer(const std::vector<T>& data, VkBufferUsageFlags usage,
                           VkBuffer& buffer, VkDeviceMemory& memory);

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void createSyncObjects();

    void recordCommandBuffers(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    VkShaderModule createShaderModule(const std::vector<unsigned char>& code);

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer& buffer,
                      VkDeviceMemory& memory);

    void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);

    VkCommandBuffer beginSingleTimeCommand();

    void endSingleTimeCommand(VkCommandBuffer& commandBuffer);

    void initWindows();

    void loop();

    void updateUniformBuffers(uint32_t imageIndex);

    void drawFrame();

    void cleanupSwapChain();

    void recreateSwapChain();

    void clearBuffers();

    void clear();

    VkSampleCountFlagBits getMaxUsableSampleCount();

    GLFWwindow* m_window = nullptr;

    static bool s_framebufferResized;

    VkSurfaceKHR m_surface;

    VkInstance m_vkInstance;

    // 存储创建的回调函数信息
    VkDebugUtilsMessengerEXT m_callBack;

    // 支持一台物理设备
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

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
    // 描述符集和描述符池
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
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

    // 顶点缓冲
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    // 索引缓存
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    // ubo缓存，每一个交换链一个
    std::vector<VkBuffer> m_uboBuffers;
    std::vector<VkDeviceMemory> m_uboBufferMemories;

    // 图片纹理
    VkImage m_textureImage;
    VkImageView m_textureImageView;
    VkDeviceMemory m_textureImageMemory;
    VkSampler m_textureSampler;
    uint32_t m_mipLevels;

    // 深度缓冲
    VkImage m_depthImage;
    VkImageView m_depthImageView;
    VkDeviceMemory m_depthImageMemory;

    // 多重采样离屏渲染的缓冲
    VkImage m_colorImage;
    VkImageView m_colorImageView;
    VkDeviceMemory m_colorImageMemory;

    std::vector<Vertex> g_vertices;
    std::vector<uint32_t> g_indices;

    std::vector<const char*> m_validationLayers{"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> m_deviceExtentions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    const uint32_t WINDOW_WIDTH = 800;
    const uint32_t WINDOW_HEIGHT = 600;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
};
static bool hasStencilComponent(VkFormat format) {
    return (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

static VkSurfaceFormatKHR
chooseBestfitSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats);

static VkPresentModeKHR
chooseBestfitPresentMode(std::vector<VkPresentModeKHR>& presentModes);

static VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pCallback);

static void
destroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT* pCallback,
                              const VkAllocationCallbacks* pAllocator);

static std::vector<char> readFile(const std::string& filename);
#endif
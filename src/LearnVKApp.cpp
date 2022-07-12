// LearnVK.cpp: 定义应用程序的入口点。
//
#include "LearnVKApp.h"
#include "vulkan/vulkan_core.h"
#include <cmath>
#include <stdexcept>
#include <unordered_map>

bool LearnVKApp::s_framebufferResized = false;
void LearnVKApp::run() { // 开始运行程序
    initWindows();
    initVK();
    loop();
    clear();
}

void LearnVKApp::initWindows() { // 初始化glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "LearnVK", nullptr,
                                nullptr);
    glfwSetFramebufferSizeCallback(m_window, frameBufferResizeCallback);
}

void LearnVKApp::initVK() { // 初始化Vulkan的设备
    createVKInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
    loadModel("viking_room/viking_room.obj");
    createTextureImage("viking_room/viking_room.png");
    createTextureImageView();
    createTextureSampler();
    createLocalBuffer(g_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      m_vertexBuffer, m_vertexBufferMemory);
    createLocalBuffer(g_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indexBuffer,
                      m_indexBufferMemory);
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
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

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extentionsName.size());
    createInfo.ppEnabledExtensionNames = extentionsName.data();
    // 填写校验层信息
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    // 在实例创建之前检查实例支持的扩展列表,
    // 只检查glfw的扩展，因为校验层扩展已经通过检查
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
    if (!enableValidationLayers)
        return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    VkResult res = createDebugUtilsMessengerEXT(m_vkInstance, &createInfo,
                                                nullptr, &m_callBack);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to setup debug callback!");
    }
}

void LearnVKApp::createSurface() {
    VkResult res =
        glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_surface);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// DebugUtilsMessage
// 被Vulkan调用的静态用户定义行为函数，在一般情况只返回VK_FALSE
VKAPI_ATTR VkBool32 VKAPI_CALL LearnVKApp::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messagesSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messgaesType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    std::cerr << "validation error:" << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void LearnVKApp::frameBufferResizeCallback(GLFWwindow* window, int width,
                                           int height) {
    s_framebufferResized = true;
}

// 获取glfw和校验层的扩展
std::vector<const char*> LearnVKApp::getRequiredExtentions() {
    uint32_t glfwExtentionCount = 0;
    const char** glfwExtentionName;
    glfwExtentionName = glfwGetRequiredInstanceExtensions(&glfwExtentionCount);
    std::vector<const char*> extentionList(
        glfwExtentionName, glfwExtentionName + glfwExtentionCount);
    if (enableValidationLayers) {
        extentionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extentionList;
}

bool LearnVKApp::checkInstanceExtentionsSupport(
    std::vector<const char*>& extentionsName) {
    uint32_t extentionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount, nullptr);
    assert(extentionCount != 0);
    std::vector<VkExtensionProperties> extentionsProperties(extentionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extentionCount,
                                           extentionsProperties.data());

#ifdef PRINT_EXTENTION_INFO
    //打印可用的扩展信息
    std::cout << "\t" << extentionsProperties.size() << std::endl;
    for (auto& properties : extentionsProperties) {
        std::cout << "\t" << properties.extensionName << std::endl;
    }
#endif

    for (const char* name : extentionsName) {
        if (std::find_if(extentionsProperties.begin(), extentionsProperties.end(),
                         [name](VkExtensionProperties val) {
                             return strcmp(val.extensionName, name) == 0;
                         })
            == extentionsProperties.end()) {
            return false;
        }
    }
    return true;
}

bool LearnVKApp::checkDeviceExtentionsSupport(VkPhysicalDevice physicalDevice) {
    uint32_t extentionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount,
                                         nullptr);
    assert(extentionCount != 0);
    std::vector<VkExtensionProperties> extentionsProperties(extentionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extentionCount,
                                         extentionsProperties.data());
#ifdef PRINT_EXTENTION_INFO
    //打印可用的扩展信息
    std::cout << "\t" << extentionsProperties.size() << std::endl;
    for (auto& properties : extentionsProperties) {
        std::cout << "\t" << properties.extensionName << std::endl;
    }
#endif
    for (const char* name : m_deviceExtentions) {
        if (std::find_if(extentionsProperties.begin(), extentionsProperties.end(),
                         [name](VkExtensionProperties val) {
                             return strcmp(val.extensionName, name) == 0;
                         })
            == extentionsProperties.end()) {
            return false;
        }
    }
    return true;
}

SwapChainSupportDetails
LearnVKApp::queryDeviceSwapChainSupport(VkPhysicalDevice physicalDevice) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface,
                                              &details.surfaceCapabilities);
    // 获取物理设备表面支持的格式
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount,
                                         nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface,
                                             &formatCount, details.formats.data());
    }
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface,
                                              &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface,
                                                  &presentModeCount,
                                                  details.presentModes.data());
    }
    return details;
}

void LearnVKApp::createSwapChain() {
    SwapChainSupportDetails details =
        queryDeviceSwapChainSupport(m_physicalDevice);
    VkSurfaceFormatKHR surfaceFormat =
        chooseBestfitSurfaceFormat(details.formats);
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
    createInfo.oldSwapchain = m_swapChain; // 允许重建交换链在渲染时进行

    QueueFamiliyIndices queueFamilyIndices =
        findDeviceQueueFamilies(m_physicalDevice);
    auto familyIndicesSet = queueFamilyIndices.familiesIndexSet;
    if (familyIndicesSet.size() != 1) { // 存在多个队列族就会出现并发申请图片资源的问题
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount =
            static_cast<uint32_t>(familyIndicesSet.size());
        createInfo.pQueueFamilyIndices =
            std::vector<uint32_t>(familyIndicesSet.begin(), familyIndicesSet.end())
                .data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount =
            0; // 如果只有一个就不存在并发问题，这里随意填写即可
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform =
        details.surfaceCapabilities.currentTransform; // 预设的缩放旋转等
    createInfo.compositeAlpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // 窗口透明混合策略
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;             // 允许被遮挡像素剔除
    createInfo.oldSwapchain = VK_NULL_HANDLE; // 重建交换链时用于指定之前的交换链

    VkResult res =
        vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    uint32_t imgCount = 0;
    // 获取交换链图像的句柄
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, nullptr);
    m_swapChainImages.resize(imgCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount,
                            m_swapChainImages.data());
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainImageExtent = extent;
}

void LearnVKApp::createImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());
    for (int i = 0; i < m_swapChainImages.size(); i++) {
        m_swapChainImageViews[i] =
            createImageView(m_swapChainImages[i], m_swapChainImageFormat,
                            VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkImageView LearnVKApp::createImageView(VkImage image, VkFormat format,
                                        VkImageAspectFlags aspectMask, uint32_t mipLevels) {
    VkImageViewCreateInfo createInfo = {};
    VkImageView imageView;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.format = format;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // 指定颜色的映射方式
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    // 指定哪部分图片资源可以被访问,这里被设置为渲染目标
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    // 显示创建 ImageView
    VkResult res = vkCreateImageView(m_device, &createInfo, nullptr, &imageView);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain image views!");
    }
    return imageView;
}

void LearnVKApp::createRenderPass() {
    // 帧缓冲附着
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = m_msaaSampleCount;
    // 颜色和深度缓冲的存取策略
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // stencil缓冲的存取策略
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 在内存中的分布方式为用作呈现方式,多重采样中为attachment,不能直接呈现

    // 帧缓冲附着解析多重采样
    VkAttachmentDescription colorAttachmentResolve = {};
    colorAttachmentResolve.format = m_swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 转换为presentKHR

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = m_msaaSampleCount;
    //深度缓冲的存取策略
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //模板缓冲的存取策略
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 附着引用
    VkAttachmentReference colorAttachReference = {};
    colorAttachReference.attachment = 0; // 表示attachment在数组中的引用索引，也是shader中片元最终输出时layout(location = 0)的索引
    colorAttachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachReference = {};
    depthAttachReference.attachment = 1;
    depthAttachReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // 引用解析
    VkAttachmentReference colorAttachResolveRef = {};
    colorAttachResolveRef.attachment = 2;
    colorAttachResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 渲染流程可能包含多个子流程，其依赖于上一流程处理后的帧缓冲内容
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // 这是一个图形渲染的子流程
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachReference;    // 颜色帧缓冲附着会被在frag中使用作为输出
    subpass.pResolveAttachments = &colorAttachResolveRef; // 指向解析的colorAttachment引用
    subpass.pDepthStencilAttachment = &depthAttachReference;

    // * 子流程依赖
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; //隐含的子流程
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 需要等待交换链读取完图像
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 为等待颜色附着的输出阶段
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // 渲染流程
    VkAttachmentDescription attachments[3] = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 3;
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    VkResult res = vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr,
                                      &m_renderPass);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void LearnVKApp::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uniformBindingInfo = {};
    uniformBindingInfo.binding = 0;
    uniformBindingInfo.descriptorCount = 1;
    uniformBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBindingInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uniformBindingInfo.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerBindingInfo = {}; // 采样器
    samplerBindingInfo.binding = 1;
    samplerBindingInfo.descriptorCount = 1;
    samplerBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBindingInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBindingInfo.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding layoutBindings[2] = {uniformBindingInfo,
                                                      samplerBindingInfo};

    VkDescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = 2;
    createInfo.pBindings = layoutBindings;
    VkResult res = vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr,
                                               &m_descriptorSetLayout);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void LearnVKApp::createGraphicsPipeline() {
    VkResult res;
    VkShaderModule vertexShaderModule = createShaderModule(VERTEX_VERT);
    VkShaderModule fragmentShaderModule = createShaderModule(FRAGMENT_FRAG);
    // 指定着色器在管线的阶段
    VkPipelineShaderStageCreateInfo vertStageCreateInfo = {};
    vertStageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageCreateInfo.pName = "main";
    vertStageCreateInfo.module = vertexShaderModule;
    vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
    fragStageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageCreateInfo.pName = "main";
    fragStageCreateInfo.module = fragmentShaderModule;
    fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {vertStageCreateInfo,
                                                       fragStageCreateInfo};

    // 获取顶点的绑定信息和属性信息
    auto bindDesc = Vertex::getBindDescription();
    auto attrDesc = Vertex::getAttributeDescriptions();
    // 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindDesc;
    vertexInputCreateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attrDesc.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attrDesc.data();

    // 顶点输入装配
    VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo = {};
    vertexAssemblyCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vertexAssemblyCreateInfo.primitiveRestartEnable =
        VK_FALSE; // 是否在与STRIP图元重启索引绘制

    //视口与剪裁,视口定义的是映射关系，剪裁是定义的显示区域
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainImageExtent.width;
    viewport.height = (float)m_swapChainImageExtent.height;
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainImageExtent;

    VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
    viewportCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.viewportCount = 1;
    viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    viewportCreateInfo.pScissors = &scissor;

    // 光栅化阶段，除了fill mode外其他光栅化方式也需要GPU特性
    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE; //是否截断片元在远近平面上
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.lineWidth = 1.0f;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;            // 剔除背面
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // 逆时针顶点序为正面
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;                  // 阴影贴图的 alias
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

    // 多重采样，启用需要GPU特性
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
    multisampleCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.sampleShadingEnable = VK_TRUE;
    multisampleCreateInfo.rasterizationSamples = m_msaaSampleCount;
    multisampleCreateInfo.minSampleShading = 0.2f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

    // 深度与模板测试
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS; // 深度值小的留下
    depthStencilState.depthBoundsTestEnable = VK_FALSE;    // 指定深度范围，这里不启用
    depthStencilState.stencilTestEnable = VK_FALSE;

    // 颜色混合
    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {}; // 全局的帧缓冲设置
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {}; // 单独的帧缓冲设置
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // 下面的帧缓冲设置实现是通过alpha混合实现半透明效果
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    // 全局帧缓冲设置
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE; // 会禁用所有混合设置
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment; // 添加单独帧缓冲设置
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    //动态状态
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR};
    dynamicStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    // 管线布局
    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = 1;
    layoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = nullptr;
    res = vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr,
                                 &m_pipelineLayout);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages; // 指定着色器阶段

    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo; // 指定输入
    pipelineCreateInfo.pInputAssemblyState =
        &vertexAssemblyCreateInfo;                           // 指定顶点装配
    pipelineCreateInfo.pViewportState = &viewportCreateInfo; // 指定视口
    pipelineCreateInfo.pRasterizationState =
        &rasterizationCreateInfo; // 指定光栅器设置
    pipelineCreateInfo.pMultisampleState =
        &multisampleCreateInfo; // 指定多重采样设置
    pipelineCreateInfo.pDepthStencilState =
        &depthStencilState; // 指定深度模板缓冲
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = m_pipelineLayout;

    pipelineCreateInfo.renderPass = m_renderPass;
    pipelineCreateInfo.subpass = 0; // 子流程数组中的索引

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // 通过已有管线创造新的管线
    pipelineCreateInfo.basePipelineIndex = -1;

    res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1,
                                    &pipelineCreateInfo, nullptr,
                                    &m_graphicsPipeline);
    // 其中pipelinecache 是管线缓存对象，加速管线创立
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    // 销毁创建的shaderModule
    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
}

void LearnVKApp::createFrameBuffers() {
    m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());
    for (int i = 0; i < m_swapChainImages.size(); i++) {
        VkImageView imageViews[] = {m_colorImageView, m_depthImageView, m_swapChainImageViews[i]}; // 写进自己创建的framebuffer对象中，然后由swapchain的对象来resolve呈现
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = m_renderPass;
        createInfo.attachmentCount = 3;
        createInfo.pAttachments = imageViews;
        createInfo.width = m_swapChainImageExtent.width;
        createInfo.height = m_swapChainImageExtent.height;
        createInfo.layers = 1;
        VkResult res = vkCreateFramebuffer(m_device, &createInfo, nullptr,
                                           &m_swapChainFrameBuffers[i]);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to create frame buffer");
        }
    }
}

void LearnVKApp::createCommandPool() {
    QueueFamiliyIndices indices = findDeviceQueueFamilies(m_physicalDevice);
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = indices.graphicsFamily;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult res =
        vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void LearnVKApp::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(
        m_swapChainImageExtent.width, m_swapChainImageExtent.height, 1,
        m_msaaSampleCount, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    transitionImageLayout(m_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1); // 优化
}

void LearnVKApp::createColorResources() {
    VkFormat colorFormat = m_swapChainImageFormat;
    createImage(m_swapChainImageExtent.width, m_swapChainImageExtent.height, 1,
                m_msaaSampleCount, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
    m_colorImageView = createImageView(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    transitionImageLayout(m_colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1); // 优化
}

VkFormat LearnVKApp::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
         VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat LearnVKApp::findSupportedFormat(const std::vector<VkFormat>& formats,
                                         VkImageTiling tiling,
                                         VkFormatFeatureFlags features) {
    for (VkFormat format : formats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void LearnVKApp::createTextureImage(const std::string& textureName) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((TEXTURE_PATH + textureName).c_str(), &texWidth,
                                &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error(
            "failed to load textures! searching:" + TEXTURE_PATH + textureName);
    }
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))));
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);
    stbi_image_free(pixels);

    createImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), m_mipLevels,
                VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage,
                m_textureImageMemory);
    transitionImageLayout(
        m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels); // oldlayout
                                                            // 在创建时我们指定为了UNDEFINED
    copyBufferToImage(stagingBuffer, m_textureImage,
                      static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    // transitionImageLayout(
    //     m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_mipLevels); // 再次将图片layout转换为着色器可以使用
    generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels); // 创建mipmaps最后会将布局转为SHADRE_READ_ONLY_OPTIMAL

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void LearnVKApp::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
                             VkImageTiling tiling, VkImageUsageFlags usage,
                             VkMemoryPropertyFlags properties, VkImage& image,
                             VkDeviceMemory& memory) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult res = vkCreateImage(m_device, &imageInfo, nullptr, &image);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memReq = {};
    vkGetImageMemoryRequirements(m_device, image, &memReq);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
    res = vkAllocateMemory(m_device, &allocInfo, nullptr, &memory);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(m_device, image, memory, 0);
}

void LearnVKApp::generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture format linear bilt not support!");
    }
    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = image;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    imageBarrier.subresourceRange.levelCount = 1;

    VkCommandBuffer commandBuffer = beginSingleTimeCommand();

    int mipWidth = static_cast<int>(texWidth);
    int mipHeight = static_cast<int>(texHeight);
    for (int i = 1; i < mipLevels; i++) {
        imageBarrier.subresourceRange.baseMipLevel = i - 1;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, // 第一个屏障将单张mipmap布局转换到最佳转换模式
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &imageBarrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // 只有前miplevels-1个会被blit执行，将image最后一个
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, // 第二个屏障将其转换为shader read最优的布局
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &imageBarrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    imageBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // 最高级的mipmap的layout已然还是上次transition的结果
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageBarrier);

    endSingleTimeCommand(commandBuffer);
}

void LearnVKApp::transitionImageLayout(VkImage image, VkFormat format,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout, uint32_t mipsLevels) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommand();
    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.srcQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED; // 如果不需要转换队列族一定要设置ignore
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = image;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = mipsLevels;

    VkPipelineStageFlags sourceStage;      // 生产者阶段，在屏障前
    VkPipelineStageFlags destinationStage; // 消费者阶段，在屏障后

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format)) {
            imageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) { // 新创建的图片转为传输layout
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask =
            VK_ACCESS_TRANSFER_WRITE_BIT; //消费者转移元素为写入

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        imageBarrier.srcAccessMask = 0;
        imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage,
                         destinationStage, // 屏障前和屏障后的管线阶段
                         0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    endSingleTimeCommand(commandBuffer);
}

void LearnVKApp::copyBufferToImage(VkBuffer buffer, VkImage image,
                                   uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommand();
    VkBufferImageCopy region = {};
    region.bufferRowLength = 0;
    region.bufferImageHeight =
        0; // 以上两参数用于设置内存对其，如果设置为0则紧凑存放
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = 0;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommand(commandBuffer);
}

void LearnVKApp::createTextureImageView() {
    m_textureImageView = createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                                         VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
}

void LearnVKApp::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR; //放大时采用线性插值方法
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; //寻址模式重复
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // 坐标系统[0, 1)
    samplerInfo.compareEnable = VK_FALSE;           // 采样时与一个值比较，结果作用于过滤操作
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);
    VkResult res =
        vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void LearnVKApp::loadModel(const std::string& modelName) {
    tinyobj::attrib_t attr;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string modelPath = MODEL_PATH + modelName;

    if (!tinyobj::LoadObj(&attr, &shapes, &materials, &warn, &err,
                          modelPath.c_str())) {
        throw std::runtime_error(err);
    }
    std::unordered_map<Vertex, uint32_t> uniqueVertexIndexMap{}; // 顶点去重
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;
            vertex.position = {
                attr.vertices[3 * index.vertex_index + 0],
                attr.vertices[3 * index.vertex_index + 1],
                attr.vertices[3 * index.vertex_index + 2],
            };
            vertex.texCoord = {attr.texcoords[2 * index.texcoord_index + 0],
                               1.0f - attr.texcoords[2 * index.texcoord_index + 1]};
            vertex.color = {1.0f, 1.0f, 1.0f};
            if (uniqueVertexIndexMap.count(vertex) == 0) {
                uniqueVertexIndexMap[vertex] = static_cast<uint32_t>(g_vertices.size());
                g_vertices.push_back(vertex);
            }
            g_indices.push_back(uniqueVertexIndexMap[vertex]);
        }
    }
}

template <typename T>
void LearnVKApp::createLocalBuffer(const std::vector<T>& info,
                                   VkBufferUsageFlags usage, VkBuffer& buffer,
                                   VkDeviceMemory& memory) {
    VkDeviceSize bufferSize = sizeof(T) * info.size();
    VkBuffer stageBuffer;
    VkDeviceMemory stageBufferMemory;
    // 创建暂存缓存区
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stageBuffer, stageBufferMemory);
    // 映射内存
    void* data; // 内存映射后的地址
    vkMapMemory(m_device, stageBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, info.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(m_device, stageBufferMemory);
    // 创建CPU不可访问的顶点缓存
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    copyBuffer(stageBuffer, buffer, bufferSize);
    vkDestroyBuffer(m_device, stageBuffer, nullptr);
    vkFreeMemory(m_device, stageBufferMemory, nullptr);
}

void LearnVKApp::copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer,
                            VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommand();
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.size = size;
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
    endSingleTimeCommand(commandBuffer);
}

VkCommandBuffer LearnVKApp::beginSingleTimeCommand() {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void LearnVKApp::endSingleTimeCommand(VkCommandBuffer& commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VkQueue queue = m_queueMap["graphicsFamily"];
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void LearnVKApp::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    // 每一个swapchain image一个ubo buffer
    size_t imagesNum = m_swapChainImages.size();
    m_uboBuffers.resize(imagesNum);
    m_uboBufferMemories.resize(imagesNum);
    for (int i = 0; i < imagesNum; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     m_uboBuffers[i], m_uboBufferMemories[i]);
    }
}

void LearnVKApp::createDescriptorPool() {
    VkDescriptorPoolSize uniformPoolSize = {};
    uint32_t imageCount = static_cast<uint32_t>(m_swapChainImages.size());
    uniformPoolSize.descriptorCount = imageCount;
    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.descriptorCount = imageCount;
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    VkDescriptorPoolSize poolSizes[2] = {uniformPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 2;
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets = imageCount;

    VkResult res =
        vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_descriptorPool);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void LearnVKApp::createDescriptorSets() {
    size_t imageCount = m_swapChainImages.size();
    std::vector<VkDescriptorSetLayout> layouts(imageCount, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(imageCount);
    VkResult res =
        vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor sets!");
    }

    for (int i = 0; i < imageCount; i++) {
        VkDescriptorBufferInfo bufferInfo = {}; // Uniform Object Buffer
        bufferInfo.buffer = m_uboBuffers[i];
        bufferInfo.range = sizeof(UniformBufferObject);
        bufferInfo.offset = 0;

        VkDescriptorImageInfo imageInfo = {}; // image sampler
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        VkWriteDescriptorSet bufferWrite = {};
        bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferWrite.dstSet = m_descriptorSets[i];
        bufferWrite.dstBinding = 0;
        bufferWrite.dstArrayElement = 0;
        bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bufferWrite.descriptorCount = 1;
        bufferWrite.pBufferInfo = &bufferInfo; // 指定缓冲

        VkWriteDescriptorSet imageWrite = {};
        imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageWrite.dstSet = m_descriptorSets[i];
        imageWrite.dstBinding = 1;
        imageWrite.dstArrayElement = 0;
        imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageWrite.descriptorCount = 1;
        imageWrite.pImageInfo = &imageInfo; //指定引用的图像

        VkWriteDescriptorSet descWrites[2] = {bufferWrite, imageWrite};
        vkUpdateDescriptorSets(m_device, 2, descWrites, 0, nullptr);
    }
}

void LearnVKApp::createCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level =
        VK_COMMAND_BUFFER_LEVEL_PRIMARY; // 设为顶级缓存，意味着可以直接被提交，但是不能被其他缓存调用
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    VkResult res =
        vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }
}

void LearnVKApp::createSyncObjects() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishSemaphore.resize(MAX_FRAMES_IN_FLIGHT);

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult res1 = vkCreateSemaphore(m_device, &createInfo, nullptr,
                                          &m_imageAvailableSemaphore[i]);
        VkResult res2 = vkCreateSemaphore(m_device, &createInfo, nullptr,
                                          &m_renderFinishSemaphore[i]);
        VkResult res3 =
            vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fences[i]);
        if (res1 != VK_SUCCESS || res2 != VK_SUCCESS || res3 != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
}

void LearnVKApp::recordCommandBuffers(VkCommandBuffer commandBuffer,
                                      uint32_t imageIndex) {
    // 让command buffer 开始记录执行指令
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo = nullptr;
    VkResult res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to begin command buffer!");
    }
    // 开始渲染流程
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_renderPass;
    renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[imageIndex];
    renderPassBeginInfo.renderArea.extent = m_swapChainImageExtent;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_swapChainImageExtent.width;
    viewport.height = (float)m_swapChainImageExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapChainImageExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer pBuffer[] = {m_vertexBuffer};
    const VkDeviceSize offsets[] = {0};
    // 开始绑定顶点缓冲
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, pBuffer, offsets);
    // 开始绑定顶点索引
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayout, 0, 1, &m_descriptorSets[imageIndex],
                            0, nullptr);
    // vkCmdDraw(commandBuffer, static_cast<uint32_t>(g_vertices.size()), 1, 0,
    // 0);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(g_indices.size()), 1, 0,
                     0, 0);
    vkCmdEndRenderPass(commandBuffer);
    res = vkEndCommandBuffer(commandBuffer);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

VkShaderModule
LearnVKApp::createShaderModule(const std::vector<unsigned char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    VkResult res =
        vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

// 查找合适的内存类型
uint32_t LearnVKApp::findMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // typeFilter要求只需要响应位域为1，
        // 然后目标的properties位域要与遍历元素完全相同
        if ((typeFilter & (1 << i)) && (properties & memProperties.memoryTypes[i].propertyFlags) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

bool LearnVKApp::checkValidationLayersProperties() {
    uint32_t validationLayersCount = 0;
    vkEnumerateInstanceLayerProperties(&validationLayersCount, nullptr);
    assert(validationLayersCount != 0);
    std::vector<VkLayerProperties> availableLayers(validationLayersCount);
    vkEnumerateInstanceLayerProperties(&validationLayersCount,
                                       availableLayers.data());
    // 检查所有m_validationLayers中要求的layer层是否都支持
    for (auto& layer : m_validationLayers) {
        if (std::find_if(availableLayers.begin(), availableLayers.end(),
                         [layer](VkLayerProperties properties) {
                             return strcmp(properties.layerName, layer) == 0;
                         })
            == availableLayers.end()) {
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
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount,
                               physicalDevices.data());
    for (auto& device : physicalDevices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            m_msaaSampleCount = getMaxUsableSampleCount();
            break;
        }
    }
    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to pick a suitable GPU!");
    }
}

// 通过获取properties 和 features然后判断是否满足需求
bool LearnVKApp::isDeviceSuitable(VkPhysicalDevice physicalDevice) {
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
    return indices.isComplete() && extentionsSupport && swapChainAdequate && features.samplerAnisotropy;
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

    // 填写物理设备features
    VkPhysicalDeviceFeatures features = {};
    features.samplerAnisotropy = VK_TRUE; // 此处我们需要启用各项异性
    features.sampleRateShading = VK_TRUE; // 开启多重采样着色

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    // 只创建一个队列
    deviceCreateInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &features;
    deviceCreateInfo.enabledExtensionCount =
        static_cast<uint32_t>(m_deviceExtentions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_deviceExtentions.data();
    // 校验层
    if (enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount =
            static_cast<uint32_t>(m_validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }
    // 创建逻辑设备
    VkResult res =
        vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }
    // 命令队列在创建逻辑设备时被一起创建，因为我们只创建了一个队列，所以直接获取索引0的队列即可
    VkQueue queue;
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.graphicsFamily), 0,
                     &queue); // 获取到队列句柄,并添加到map中去
    m_queueMap.insert(std::make_pair("graphicsFamily", queue));
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.presentFamily), 0,
                     &queue);
    m_queueMap.insert(std::make_pair("presentFamily", queue));
}

QueueFamiliyIndices
LearnVKApp::findDeviceQueueFamilies(VkPhysicalDevice physicalDevice) {
    QueueFamiliyIndices indices;
    uint32_t deviceQueueFamilyCount = 0;
    VkBool32 presentSupport = false;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &deviceQueueFamilyCount, nullptr);
    assert(deviceQueueFamilyCount != 0);
    std::vector<VkQueueFamilyProperties> properties(deviceQueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &deviceQueueFamilyCount, properties.data());

    for (uint32_t i = 0; i < deviceQueueFamilyCount; i++) {
        VkQueueFamilyProperties queueFamily = properties[i];
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface,
                                             &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
            break;
    }
    indices.familiesIndexSet.insert(indices.graphicsFamily);
    indices.familiesIndexSet.insert(indices.presentFamily);
    return indices;
}

void LearnVKApp::loop() { // 应用的主循环
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(m_device);
}

void LearnVKApp::updateUniformBuffers(uint32_t imageIndex) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), 0.0f,        // time * glm::radians(90.0f),
                            glm::vec3(0.0f, 0.0f, 1.0f)); // 以Z轴为轴每秒旋转90°
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0),
                           glm::vec3(0.0f, 0.0f, 1.0f)); // 从(2,2,2)看向(0,0,0)
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                m_swapChainImageExtent.width / static_cast<float>(m_swapChainImageExtent.height),
                                0.1f, 10.0f); // 投影矩阵，fov:45 平截头体近0.1远10
    ubo.proj[1][1] *= -1;                     // 因为OpenGL与Vulkan的y轴正方向是反的，因此需要将y轴缩放系数取相反数

    void* data;
    vkMapMemory(m_device, m_uboBufferMemories[imageIndex], 0, sizeof(ubo), 0,
                &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(m_device, m_uboBufferMemories[imageIndex]);
}

void LearnVKApp::drawFrame() {
    vkWaitForFences(
        m_device, 1, &m_fences[m_currentFrameIndex], VK_TRUE,
        MAX_TIMEOUT); // 等待某个预渲染的帧被GPU处理完毕，通过栅栏，实现不会提交过多的帧

    // 从交换链获取一张图像
    uint32_t imageIndex;
    VkSemaphore waitSemaphores[] = {
        m_imageAvailableSemaphore
            [m_currentFrameIndex]}; // 为等待从交换链获取图片的信号量
    VkSemaphore signalSemaphores[] = {
        m_renderFinishSemaphore[m_currentFrameIndex]};
    VkResult res = vkAcquireNextImageKHR(
        m_device, m_swapChain, MAX_TIMEOUT, waitSemaphores[0], VK_NULL_HANDLE,
        &imageIndex); //开始获取的同时 P(wait);当获取之后就会S(wait);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain!");
    }
    vkResetFences(
        m_device, 1,
        &m_fences
            [m_currentFrameIndex]); // 后延fence的重置表示如果重建了swapChain已然可以进入这一帧
    updateUniformBuffers(imageIndex);
    vkResetCommandBuffer(m_commandBuffers[m_currentFrameIndex], 0);
    recordCommandBuffers(m_commandBuffers[m_currentFrameIndex], imageIndex);
    // 对帧缓冲附着执行指令缓冲中的渲染指令
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStageFlags[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores =
        waitSemaphores; // P(wait); 在获取到图片S(wait)就submit指令
    submitInfo.pWaitDstStageMask = waitStageFlags;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores =
        signalSemaphores; // S(signal); 代表完成渲染，可以呈现
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrameIndex];
    VkQueue& queue = m_queueMap["graphicsFamily"];
    res = vkQueueSubmit(
        queue, 1, &submitInfo,
        m_fences[m_currentFrameIndex]); // 提交渲染指令的时候一并提交这一帧的栅栏
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to submit command buffer!");
    }
    // 返回渲染后的图像到交换链进行呈现操作
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores =
        signalSemaphores; // P(signal); 申请将渲染的内容呈现
    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    queue = m_queueMap["presentFamily"];
    res = vkQueuePresentKHR(queue, &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || s_framebufferResized) {
        recreateSwapChain();
        s_framebufferResized = false;
    } else if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to present to surface!");
    }
    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void LearnVKApp::cleanupSwapChain() {
    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    vkDestroyImage(m_device, m_depthImage, nullptr);
    vkFreeMemory(m_device, m_depthImageMemory, nullptr);

    vkDestroyImageView(m_device, m_colorImageView, nullptr);
    vkDestroyImage(m_device, m_colorImage, nullptr);
    vkFreeMemory(m_device, m_colorImageMemory, nullptr);

    for (auto& frameBuffer : m_swapChainFrameBuffers) {
        vkDestroyFramebuffer(m_device, frameBuffer, nullptr);
    }
    vkFreeCommandBuffers(m_device, m_commandPool,
                         static_cast<uint32_t>(m_commandBuffers.size()),
                         m_commandBuffers.data());
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    for (auto& imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}

void LearnVKApp::recreateSwapChain() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    // 清理
    vkDeviceWaitIdle(m_device);
    cleanupSwapChain();
    // 重建
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFrameBuffers();
    createCommandBuffers();
}

void LearnVKApp::clearBuffers() {
    for (int i = 0; i < m_swapChainImages.size(); i++) {
        vkDestroyBuffer(m_device, m_uboBuffers[i], nullptr);
        vkFreeMemory(m_device, m_uboBufferMemories[i], nullptr);
    }
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
}

void LearnVKApp::clear() { // 释放Vulkan的资源
    if (enableValidationLayers) {
        destroyDebugUtilsMessengerEXT(m_vkInstance, &m_callBack, nullptr);
    }
    cleanupSwapChain();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishSemaphore[i], nullptr);
        vkDestroyFence(m_device, m_fences[i], nullptr);
    }
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    vkDestroySampler(m_device, m_textureSampler, nullptr);
    vkDestroyImageView(m_device, m_textureImageView, nullptr);
    vkDestroyImage(m_device, m_textureImage, nullptr);
    vkFreeMemory(m_device, m_textureImageMemory, nullptr);

    clearBuffers();

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

VkExtent2D
LearnVKApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void LearnVKApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult res = vkCreateBuffer(m_device, &createInfo, nullptr, &buffer);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memReq = {};
    vkGetBufferMemoryRequirements(m_device, buffer, &memReq);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties);
    res = vkAllocateMemory(m_device, &allocInfo, nullptr, &memory);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(m_device, buffer, memory, 0);
}

VkSampleCountFlagBits LearnVKApp::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

// 选择一个最好的格式，如果有SRGB就选SRGB
VkSurfaceFormatKHR
chooseBestfitSurfaceFormat(std::vector<VkSurfaceFormatKHR>& formats) {
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    } else {
        for (auto& format : formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
    }
    return formats[0];
}

// 选择一个最好的显示模式，有三缓冲会直接选择三缓冲
VkPresentModeKHR
chooseBestfitPresentMode(std::vector<VkPresentModeKHR>& presentModes) {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (auto& presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        } else if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
            bestMode = VK_PRESENT_MODE_FIFO_KHR;
        }
    }
    return bestMode;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pCallback) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT* pCallback,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, *pCallback, pAllocator);
    }
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

int main() {
    LearnVKApp app;
    try {
        app.run();
    } catch (const std::exception& e) { // Vulkan主循环中出现的异常在这里被捕获
        std::cerr << e.what() << std::endl;
    }
}

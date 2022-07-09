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
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFrameBuffers();
	createCommandPool();
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
	uint32_t imgCount = 0;
	// 获取交换链图像的句柄
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, nullptr);
	m_swapChainImages.resize(imgCount);
	vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, m_swapChainImages.data());
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainImageExtent = extent;
}

void LearnVKApp::createImageViews() {
	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (int i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		VkImage image = m_swapChainImages[i];
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.format = m_swapChainImageFormat;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		// 指定颜色的映射方式
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		// 指定哪部分图片资源可以被访问,这里被设置为渲染目标
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		// 显示创建 ImageView
		VkResult res = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain image views!");
		}
	}
}

void LearnVKApp::createRenderPass() {
	// 帧缓冲附着
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// 颜色和深度缓冲的存取策略
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// stencil缓冲的存取侧率
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// 在内存中的分布方式为用作呈现

	// 附着引用
	VkAttachmentReference colorAttachReference = {};
	colorAttachReference.attachment = 0;	// 表示description在数组中的引用索引
	colorAttachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// 渲染流程可能包含多个子流程，其依赖于上一流程处理后的帧缓冲内容
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// 这是一个图形渲染的子流程
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachReference;	// 颜色帧缓冲附着会被在frag中使用作为输出

	// * 子流程依赖
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;//隐含的子流程
	dependency.dstSubpass = 0; 
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 需要等待交换链读取完图像
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 为等待颜色附着的输出阶段
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// 渲染流程
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;
	VkResult res = vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_renderPass);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void LearnVKApp::createGraphicsPipeline() {
	VkResult res;
	VkShaderModule vertexShaderModule = createShaderModule(VERTEX_VERT);
	VkShaderModule fragmentShaderModule = createShaderModule(FRAGMENT_FRAG);
	// 指定着色器在管线的阶段
	VkPipelineShaderStageCreateInfo vertStageCreateInfo = {};
	vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStageCreateInfo.pName = "main";
	vertStageCreateInfo.module = vertexShaderModule;
	vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
	fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStageCreateInfo.pName = "main";
	fragStageCreateInfo.module = fragmentShaderModule;
	fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineShaderStageCreateInfo shaderStages[2] = {
		vertStageCreateInfo, fragStageCreateInfo
	};

	// 顶点输入
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

	// 顶点输入装配
	VkPipelineInputAssemblyStateCreateInfo vertexAssemblyCreateInfo = {};
	vertexAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	vertexAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vertexAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;	// 是否在与STRIP图元重启索引绘制

	//视口与剪裁,视口定义的是映射关系，剪裁是定义的显示区域
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainImageExtent.width;
	viewport.height = (float)m_swapChainImageExtent.height;
	viewport.maxDepth = 1.0f;
	viewport.minDepth = 0.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainImageExtent;

	VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	// 光栅化阶段，除了fill mode外其他光栅化方式也需要GPU特性
	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
	rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;	//是否截断片元在远近平面上
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.lineWidth = 1.0f;
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// 剔除背面
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// 顺时针顶点序为正面
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;	// 阴影贴图的 alias
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

	// 多重采样，启用需要GPU特性
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.pSampleMask = nullptr;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	// 深度与模板测试
	// VkPipelineDepthStencilStateCreateInfo depthCreateInfo = {};

	// 颜色混合
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};	// 全局的帧缓冲设置
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};	// 单独的帧缓冲设置
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;	// 会禁用所有混合设置
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachment;	// 添加单独帧缓冲设置
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;
	
	//动态状态
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	VkDynamicState dynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	// 管线布局
	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pSetLayouts = nullptr;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;
	res = vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_pipelineLayout);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;	// 指定着色器阶段
	
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;	// 指定输入
	pipelineCreateInfo.pInputAssemblyState = &vertexAssemblyCreateInfo;	// 指定顶点装配
	pipelineCreateInfo.pViewportState = &viewportCreateInfo;	// 指定视口
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo; // 指定光栅器设置
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo; // 指定多重采样设置
	pipelineCreateInfo.pDepthStencilState = nullptr;	// 指定深度模板缓冲
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = m_pipelineLayout;

	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.subpass = 0;	// 子流程数组中的索引

	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// 通过已有管线创造新的管线
	pipelineCreateInfo.basePipelineIndex = -1;
	
	res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);
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
		VkImageView imageViews[] = { m_swapChainImageViews[i] };
		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = imageViews;
		createInfo.width = m_swapChainImageExtent.width;
		createInfo.height = m_swapChainImageExtent.height;
		createInfo.layers = 1;
		VkResult res = vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_swapChainFrameBuffers[i]);
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
	VkResult res = vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void LearnVKApp::createCommandBuffers() {
	m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	VkResult res = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
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
		VkResult res1 = vkCreateSemaphore(m_device, &createInfo, nullptr, &m_imageAvailableSemaphore[i]);
		VkResult res2 = vkCreateSemaphore(m_device, &createInfo, nullptr, &m_renderFinishSemaphore[i]);
		VkResult res3 = vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fences[i]);
		if (res1 != VK_SUCCESS || res2 != VK_SUCCESS || res3 != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void LearnVKApp::recordCommandBuffers(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	VkClearValue clearValue = {0,0,0,1.0f};
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainImageExtent.width;
	viewport.height = (float)m_swapChainImageExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainImageExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
	res = vkEndCommandBuffer(commandBuffer);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

VkShaderModule LearnVKApp::createShaderModule(const std::vector<unsigned char>& code) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shaderModule;
	VkResult res = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
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
	
	// 填写物理设备features
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
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		if (presentSupport) {
			indices.presentFamily = i;
		}
		if (indices.isComplete()) break;
	}
	indices.familiesIndexSet.insert(indices.graphicsFamily);
	indices.familiesIndexSet.insert(indices.presentFamily);
	return indices;
}


void LearnVKApp::loop() {	// 应用的主循环
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		drawFrame();
	}
	vkDeviceWaitIdle(m_device);
}

void LearnVKApp::drawFrame() {
	vkWaitForFences(m_device, 1, &m_fences[m_currentFrameIndex], VK_TRUE, MAX_TIMEOUT);		// 等待某个预渲染的帧被GPU处理完毕，通过栅栏，实现不会提交过多的帧
	vkResetFences(m_device, 1, &m_fences[m_currentFrameIndex]);
	// 从交换链获取一张图像
	uint32_t imageIndex;
	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrameIndex]};		// 为等待从交换链获取图片的信号量
	VkSemaphore signalSemaphores[] = { m_renderFinishSemaphore[m_currentFrameIndex]};
	vkAcquireNextImageKHR(m_device, m_swapChain, MAX_TIMEOUT, waitSemaphores[0], VK_NULL_HANDLE, &imageIndex);	//开始获取的同时 P(wait);当获取之后就会S(wait);

	vkResetCommandBuffer(m_commandBuffers[m_currentFrameIndex], 0);
	recordCommandBuffers(m_commandBuffers[m_currentFrameIndex], imageIndex);
	// 对帧缓冲附着执行指令缓冲中的渲染指令
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkPipelineStageFlags waitStageFlags[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;	// P(wait); 在获取到图片S(wait)就submit指令
	submitInfo.pWaitDstStageMask = waitStageFlags;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;	// S(signal); 代表完成渲染，可以呈现
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrameIndex];
	VkQueue& queue = m_queueMap["graphicsFamily"];
	VkResult res = vkQueueSubmit(queue, 1, &submitInfo, m_fences[m_currentFrameIndex]);	// 提交渲染指令的时候一并提交这一帧的栅栏
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to submit command buffer!");
	}
	// 返回渲染后的图像到交换链进行呈现操作
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;		
	presentInfo.pWaitSemaphores = signalSemaphores;		// P(signal); 申请将渲染的内容呈现
	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	queue = m_queueMap["presentFamily"];
	res = vkQueuePresentKHR(queue,  &presentInfo);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to present to surface!");
	}
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void LearnVKApp::clear() {	// 释放Vulkan的资源
	if (enableValidationLayers) {
		destroyDebugUtilsMessengerEXT(m_vkInstance, &m_callBack, nullptr);
	}
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_device, m_imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(m_device, m_renderFinishSemaphore[i], nullptr);
		vkDestroyFence(m_device, m_fences[i], nullptr);
	}
	
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	for (auto& frameBuffer : m_swapChainFrameBuffers) {
		vkDestroyFramebuffer(m_device, frameBuffer, nullptr);
	}
	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);
	for (auto& imageView : m_swapChainImageViews) {
		vkDestroyImageView(m_device, imageView, nullptr);
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
			VK_FORMAT_B8G8R8A8_SRGB,
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
		};
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


int main()
{
	LearnVKApp app;
	try {
		app.run();
	} catch (const std::exception& e) {		// Vulkan主循环中出现的异常在这里被捕获
		std::cerr << e.what() << std::endl;
	}
}

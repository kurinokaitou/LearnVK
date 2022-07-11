# Vulkan学习笔记

本系列文章是个人学习https://vulkan-tutorial.com Vulkan教程的学习笔记，主要为了梳理Vulkan API各种复杂概念和状态记录了教程中重点的备忘，文中如有疏漏的部分可以去看译文和原文的内容。在这里也非常感谢fangcun010的翻译与分享。

[Vulkan Tutorial EN]: https://vulkan-tutorial.com/
[Vulkan Tutorial CN]: https://github.com/fangcun010/VulkanTutorialCN

## 环境配置

本文中开发环境的配置与教程中有所差别，不过基本上都是配置相同的依赖库，包括Vulkan SDK、glfw和glm等。具体的C++和Vulkan开发环境网络上详细的教程已经有很多，这里提供一种比较简单的方式。

我使用的是Visual Studio 2022 + CMake + vcpkg的方式配置的本教程的开发环境。CMake和VS自不用说，VS对CMake跨平台工程的构建提供了非常多的支持，这里主要引入的是vcpkg C++包管理器和两者使用上的配合。

**安装vcpkg并安装依赖库**

安装vcpkg之前需要确保安装了最新的CMake和PowerShell，否则会自动下载，而且它的默认下载源网速基本上很难下载成功。安装好vcpkg之后在环境变量中添加`VCPKG_ROOT_PATH`代表安装的根目录之后，通过命令行可以用`./vcpkg search`搜索想要安装的包。这里我们要安装的是`glm`和`glfw`，分别是常用的矩阵向量数学库和GUI框架库，安装两个库的命令如下：

```bash
./vcpkg install glm:x64-windows
./vcpkg install glfw:x64-windows
```

之后只需要通过在CMakeLists.txt中使用`find_package`就可以引入项目，甚至也不需要vcpkg集成到VS，可以说非常方便。

**安装Vulkan SDK**

Vulkan SDK在vcpkg里的版本比较旧，而且本身Vulkan SDK也有一个自己的安装管理器来自定义安装，所以我们还是使用直接到LunarG官网https://vulkan.lunarg.com/sdk/home下载SDK的方式来配置Vulkan。下载安装除了本体还可以选择（VMA）Vulkan Memory Allocator来帮助管理Vulkan程序的内存。

**通过CMake来构建项目**

这里直接给出一个简单项目的CMakeLists.txt，在VS中创建CMake项目之后将原始CMakeLists.txt文件替换即可。

```cmake
cmake_minimum_required (VERSION 3.8)
# 设置cmake工具链为vcpkg
if(NOT DEFINED ENV{VCPKG_ROOT_PATH})
	message(FATAL_ERROR "VCPKG_ROOT_PATH not defined!")
endif()
set(VCPKG_PATH $ENV{VCPKG_ROOT_PATH})
set(VCPKG_ROOT ${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake CACHE PATH "")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})

# 项目设置
project ("LearnVK")	# 替换为自己的项目名称
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Vulkan SDK设置
if(NOT DEFINED ENV{VK_SDK_PATH})
	message(FATAL_ERROR "VK_SDK_PATH not defined!")
endif()
set(VK_SDK_PATH $ENV{VK_SDK_PATH})
set(VK_SDK_INCLUDE ${VK_SDK_PATH}/Include)
set(VK_SDK_LIB ${VK_SDK_PATH}/Lib/vulkan-1.lib)

# vcpkg安装的glm和glfw3引入包
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# 将源代码添加到此项目的可执行文件
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR} "include/*.h")
file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR} "src/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})
add_executable(LearnVK ${SOURCE_FILES})

# 链接到库文件
target_link_libraries(LearnVK PRIVATE glm::glm)
target_link_libraries(LearnVK PRIVATE glfw)
target_link_libraries(LearnVK PRIVATE ${VK_SDK_LIB})

# 添加包含目录
target_include_directories(LearnVK PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(LearnVK PRIVATE ${VK_SDK_INCLUDE})
```

配置完开发环境，就可以正式开始Vulkan的学习了。



## 初始Vulkan：画一个三角形

### 渲染流程概述

1. 创建`VkInstance`实例和选择物理设备`VkPhysicalDevice`
2. 创建逻辑设备`VkDevice`然后指定使用的队列族：一个队列族是一个特定的操作集合。
3. 窗口步骤和交换链：窗口可以使用`glfw`。对于窗口的渲染，需要用的窗口表面`VkSurfaceKHR`和`VkSwapChainKHR`。
4. 图形视图和帧缓冲。

```mermaid
flowchart LR
	 VkFrameBuffer --> VkImageView  --> Image
```

5. 渲染流程，渲染一个三角形只需要渲染到一张图像的颜色目标即可(渲染目标），渲染前先创建并清除`FrameBuffer`。

6. 图形管线。`VkPipeline`描述的是显卡可以配置的状态。比如其中`VkShaderModule`对象的可编程状态。图形管线的配需要在渲染前完成，意味着重新使用另外的着色器或是顶点布局需要重新创建整个图形管线。因此我们的策略是**提前创建你好所有需要的图形管线**，这样切换不同的管线也会更快，并且渲染预期效果也会更容易。

7. 指令池和指令缓冲。所有操作被提交到`VkCommandBuffer`对象，然后提交给队列。这个对象由`VkCommandPool`分配。这些指令包括开始渲染、绑定管线、真正绘制、结束渲染。每个指令缓冲对应交换链中的一张图像，需要在主循环开始前分配。

8. 主循环：

   1. 首先使 用`vkAcquireNextImageKHR`函数从交换链获取一张图像。**（获取）**
   2. 接着使用`vkQueueSubmit`函数提交图像对应的指令缓冲。**（提交）**
   3. 最后，使用`vkQueuePresentKHR`函数将图像 返回给交换链，显示图像到屏幕**（返回）**

   这一期间提交队列的操作会被异步执行，因此需要信号量来同步。上面三部必须按照顺序进行。

下面给出一张Vulkan的对象图作为学习时对Vulkan结构的参考：

![vulkan_objects](media\vulkan_objects.jpg)


### Vulkan编码规范

*  Vulkan的函数都带有一个小写的vk前缀，枚举和结构体名带有一个Vk前缀，枚举值带有一个VK_前缀。Vulkan对结构体非常依赖，大量函数的参数由结构体提供，类似于DX的Description，通过填入结构体为指定的API设置参数。 
*  所有的API都会返回一个VkResult来表示成功与否，错误会返回不同的代码。这就需要我们处理好API的异常，配合后面提到的校验层建立一套异常处理机制。
* Vulkan并没有使用RAII机制，也就是说我们申请的VK资源全部需要手动进行释放。Vulkan的一个核心思想就是通过显式地定义每一个操作来避免出现不一致的现象。当然我们仍然可以使用智能指针和RAII机制来帮助我们管理所有的VK资源。

### Vulkan校验层

为了高性能低驱动的特性，Vulkan在发生硬件错误时会直接崩溃。因此在开发的时候需要一层Vulkan的校验层供调试，在官方的SDK中给出了一种校验层的实现。

## 一个基础的Vulkan Application类

### 框架代码

```c++
class LearnVKApp
{
public:
	void run();	// 开始运行

private:
	void initVK();	// 初始化Vulkan实例、包括检测扩展、校验层、初始化debug等	

	void initWindows();	 // 初始化glfw的窗口
	
	void loop();	// 主循环

	void clear(); 	// 释放所有申请的资源

	GLFWwindow* m_window = nullptr;

	VkInstance m_vkInstance;

	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
};
```



### 初始化Vulkan实例

```c++
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
	// 在实例创建之前检查实例支持的扩展列表
	if (!checkExtentionsProperties(extentionsName)) {
		throw std::runtime_error("failed to support instance extention");
	}
	VkResult res = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create vk instance");
	}
}
```

其中包括填写应用引擎信息、检验扩展信息等

### 校验层设置

Vulkan的校验层主要负责检查参数是否合法、追踪内存、log之类的工作。一般是对真正进行工作的API进行封装。



### 使用LunarG提供的校验层

我们使用LunarG校验层只需要在`VkIntanceCreateInfo`中填入`ppEnabledLayesName`和`enabledExtensionCount`即可。不过在创建真正的实例之前，和检查扩展一样，LunarG校验层也需要使用`vkEnumerateInstanceLayerProperties`来查找我们需求的Layer名字是否可用。如果可用的话就可以创建实例。

```c++
bool LearnVKApp::checkValidationLayersProperties() {
	uint32_t validationLayersCount = 0;
	vkEnumerateInstanceLayerProperties(&validationLayersCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(validationLayersCount);
	vkEnumerateInstanceLayerProperties(&validationLayersCount, availableLayers.data());
	// 检查所有m_validationLayers中要求的layer层是否都支持
	for (auto& layer : m_validationLayers) {
		if (std::find_if(availableLayers.begin(), availableLayers.end(), [layer](VkLayerProperties properties) {
			return strcmp(properties.layerName, layer) == 0;
			}) == availableLayers.end()) {
			return false;
		}
	}
	return true;
}
```

当然有了校验层之后如果没有添加任何信息的回调函数的话也不会知道校验层发出的信息，因此需要用`vkCreateDebugUtilsMessengerEXT`设置debugCallback才能实现接受Vulkan校验层发出的错误。这个函数需要我们启用扩展`VK_EXT_debug_utils`。因此需要在之前创建实例时通过在`extentsName`添加`VK_EXT_DEBUG_UTILS_EXTENSION_NAME`加入这一扩展。

因为是扩展函数不能被Vulkan直接调用，因此我们需要设置其代理函数：通过`vkGetInstantceProcAddr`获取扩展的函数地址来调用这个函数。具体的代理函数使用方式如下：

```c++
VkResult createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        // 根据函数名获取函数地址，使用这个api必须在实例中加入VK_EXT_debug_utils扩展
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
```

最后因为我们通过代理创建了一个`VkDebugUtilsMessengerEXT`的实例，因此也需要显示地使用另一个销毁它的代理函数释放它的内存，否则就会发生内存泄露在关闭页面时产生报错。

在设置完毕所有校验层和debugUtilsMessenger并且正确完成对扩展和校验层的建言，之后如果能够正常打开窗口，则说明所有的设置都是正确的。



## 物理设备与队列族

### 设备检测

首先我们需要选取一个可用的GPU设备，和之前查找扩展的方式相同，依然是通过遍历所有设备查找可用设备选取。这里我们只选取第一台可用设备，但是Vulkan是支持多设备GPU工作的。

```c++
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
```

### 设备需求检测

设备主要包含两种属性properties和features，分别代表设备基础信息如名称、类型、支持的Vulkan版本等和设备的特性如是否支持纹理压缩、64位浮点等。这些属性信息需要根据我们的应用对硬件的需求去查找，并将逻辑写在`isDeviceSuitable`中。

获取properties和features的方式如下：

```c++
vkGetPhysicalDeviceProperties(physicalDevice, &properties);
vkGetPhysicalDeviceFeatures(physicalDevice, &features);
```

获取到之后即可对设备的要求进行查找比对，如果找到符合要求的设备就返回true。

### 获取队列族

Vulkan的种种操作指令基本上都是通过提交到队列的方式实现的，而不同指令会提交到不同的队列族。对于不同的硬件设备它所支持的队列族也是不同的，因此也就要求我们找到一个满足队列族需求的硬件设备从而创建队列族和逻辑设备。根据上述描述，我们需要三个函数来完成这些逻辑：

* `isDeviceSuitable`在这里我们需要判断是否已有的物理设备满足条件。
* `findDeviceQueueFamilies`判断当前物理设备是否满足队列族的条件，如果找到一个满足条件的队列族，则返回队列族的索引。教程中队列族的条件只有一条，只要`queueFlag`有`VK_QUEUE_GRAPHICS_BIT`位的标记。
* `createLogicalDevice`当获取了符合的队列族索引，就可以根据索引创建这个队列。创建队列的过程是在创建逻辑设备时一并完成的。创建逻辑设备时除了队列，还需要设置校验层。设备校验层设置与实例校验层基本一致。而且我们已经完成了校验层的检查，这里就不需要再次检查了。最后创建了逻辑设备通过`vkGetDeviceQueue`将其队列保存到成员变量句柄中以备以后使用。



## 窗口表面

Vulkan的实现与平台无关，因此需要第三方的窗口来显示渲染的内容。Vulkan提供了与外部窗口的接口扩展`VK_KHR_surface`建立了窗口与Vulkan系统的连接，它暴露的API`VkSurfaceKHR`就是抽象的窗口表面。

这里我们使用的是`glfw`作为我们的窗口库，它已经帮我们封装好了相关的API，我们可以通过`glfwCreateWindowSurface`获得surface。

**呈现支持**：不一定所有设备都支持Vulkan的特性，而且支持绘制和表现的队列族也可能不同（不过大部分情况相同，代码中如果相同就使用同一个队列族），因此我们需要在查询设备支持的队列族中添加呈现队列(`presentQueue`)。修改`QueueFamiliyIndices`如下：

```c++
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
```

为此我们要为每一个队列族在创建逻辑设备的时候创建一个队列，修改创建逻辑设备的填写队列信息部分的代码：

```c++
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
```

创建好逻辑设备后，通过`vkGetDeviceQueue`获取队列，并根据名字保存到map中备用

```c++
VkQueue queue;
vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.graphicsFamily), 0, &queue);	// 获取到队列句柄,并添加到map中去
m_queueMap.insert(std::make_pair("graphicsFamily", queue));
vkGetDeviceQueue(m_device, static_cast<uint32_t>(indices.presentFamily), 0, &queue);
m_queueMap.insert(std::make_pair("presentFamily", queue));
```



## 交换链

不同于OpenGL有默认的帧缓冲。Vulkan想要在创建的Surface绘制需要我们自己创建交换链。交换链和`VK_KHR_Surface`一样，也是一个Vulkan的扩展。不过与Surface是Instance扩展不同，它是一个Device的扩展。我们可以通过`vkEnumerateDeviceExtensionProperties`获取到所有当前设备支持的Device扩展。

因为是扩展，所以需要显示地对swap chain进行物理设备的扩展检测之后再创建。我们需要再次修改`isSuitableDevice`来考察设备是否支持交换链。

```c++
// 查询设备是否支持要求的扩展
bool extentionsSupport = checkDeviceExtentionsSupport(physicalDevice);
bool swapChainAdequate = false;
if (extentionsSupport) {
	auto swapChainDetails = queryDeviceSwapChainSupport(physicalDevice);
	swapChainAdequate = !swapChainDetails.formats.empty() && !swapChainDetails.presentModes.empty();
}
```

同时，swap chain的创建需要三大重要参数`PresentMode`和`Format`以及`Extent2D`的支持，他们是swap chain的呈现模式，以及所承载地图片的格式色彩空间和规格大小。为了保证最佳的呈现模式以及格式，以及他们的平台支持性，我们需要编写`queryDeviceSwapChainSupport`查询设备的支持。

```c++
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
```

然后分别编写选择呈现模式、格式以及规格大小的函数确定最后使用的最优方案。选择好最优方案之后，在创建的交换链时也有一个地方需要注意。因为swap chain在vulkan程序中被创建之后是作为资源存在，通过`VkQueue`发送执行命令完成绘制。因为在之前窗口表面支持部分添加了`presentFamily`队列族，多个队列族中的队列访问资源这就有了并发性以及图像所有权上的问题。因此在创建swap chain时，需要指定不同的队列族在访问swap chain时为协同模式。

```c++
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
```

* `VK_SHARING_MODE_EXCLUSIVE`：一张图像同一时间只能被一个队列族所拥有。在另一队列族使用前，必须显式改变图像所有权。这一模式下性能表现最佳。 

* `VK_SHARING_MODE_CONCURRENT`：图像可以在多个队列族间 使用，不需要显式地改变图像所有权。 

然后就是指定一些之前创建的swap chain的参数，然后通过`vkCreateSwapchainKHR`创建交换链，然后获取其中所有图片的句柄保存以备之后使用。同时还需要保存图像的格式和规格大小。

```c++
uint32_t imgCount = 0;
// 获取交换链图像的句柄
vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, nullptr);
m_swapChainImages.resize(imgCount);
vkGetSwapchainImagesKHR(m_device, m_swapChain, &imgCount, m_swapChainImages.data());
m_swapChainImageFormat = surfaceFormat.format;
m_swapChainImageExtent = extent;
```



## 图像视图

图像视图描述了访问图像的方式，以及图像的哪一部分可以被访问。比如，图像可以被图像视图描述为一个没有细化级别的二维深度纹理，进而可以在其上进行与二维深度纹理相关的操作。 我们可以通过使用`vkCreateImageView`来对swap chain中获得的图像句柄进行设置图像视图。



## 图形管线

![图形渲染管线](media\vulkan_pipeline.png)

在关注着色器前，首先需要明确图形管线的概念。上图中黄色的部分区域是可编程阶段，包括熟悉的顶点着色器、曲面细分、几何着色器、片元着色器等；而绿色部分则是固定功能阶段，可以通过设置参数来进行改变。

Vulkan的图形管线与DirectX和OpenGL中的管线本身没有区别。我们可以按照之前学习OpenGL的理解去看Vulkan是如何构建管线的。不过值得注意的是，对于Vulkan来说图形管线是不允许动态设置的。包括着色器代码，绑定帧缓冲，混合方式的参数等等都是在创建只是就完全定死的。想要改变状态就需要切换或是创建新的管线。尽管这样在编程时会很麻烦，但是这也是Vulkan性能优化的源头所在。

## 着色器模块

Vulkan在着色器上使用的是SPIR-V，它是一种字节码格式的代码，可以用于编写图形和计算着色器。同时它也作为一门中间码，可以在传统的shader language 如 glsl和hlsl中相互转换。因为传统的glsl和hlsl在编译时需要类C编译器把高级语言编译为字节码，但是因为GPU厂商编译器实现不同，还会有错误等原因，Vulkan采用的是SPIR-V这种字节码直接作为着色器代码。

但是我们仍然可以用glsl来作为编写着色器的语言。通过Vulkan SDK中的` glslangValidator`将我们的.glsl代码编译为.spv的二进制文件，从而让Vulkan验证我们代码是否符合标准。

下面给出基础的着色器代码：

```glsl
// 顶点着色器
#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex{
	vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));

vec3 colors[3] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

void main()
{
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}

// 片元着色器
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(fragColor, 1.0);
}
```

以上的着色器代码甚至不需要定点的输入，顶点位置和颜色都硬编码在了着色器中。在之后的流程中，我们会逐渐使用图形管线中顶点输入的顶点来作为顶点着色器的输入。代码标准上，与OpenGL中编写的GLSL代码也有所不同：

* 为了让着色器代码在Vulkan下可以工作，需要添加`#extension GL_ARB_separate_shader_objects : enable`扩展
* 着色器中无论是输入还是输出都需要用`(layout = 0)`来指定对应的某个帧缓冲或是顶点输入

### 编译着色器

使用` glslangValidator`将我们的.glsl代码编译为.spv的二进制文件，可以通过以下命令行完成：

```bash
glslValidator -I <includeDir> -V100 -o <outputDir> 
```

其中`includeDir`代表编译时的包含目录，如glsl的头文件等；`outputDir`代表输出目录。这点与g++等编译器的命令行一致。`V100`代表创建Vulkan使用的spv二进制文件，其中100代表输入着色器代码的版本，编译中会以`#define Vulkan 100`这样的形式来区分编译的版本。

### 在项目中使用编译后的着色器代码

编译后的着色器就是.spv的二进制文件。我们可以通过文件读入的方式将其以`std::vector<char>`的形式载入内存。这种方式对于项目中一些动态添加的着色器比较适合。

```c++
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
```

除此之外还有一种将二进制文件硬编码为.h文件在项目构建阶段就直接嵌入工程的方式使用。这里我们借助CMake的`add_custom_command`的强大功能，在项目构建期就将编译后的代码以头文件的方式内嵌项目中。

首先在`CMakeLists.txt`中先收集项目中的着色器代码，在其中添加下面语句：

```cmake
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.comp" "${SHADER_DIR}/*.geom" "${SHADER_DIR}/*.tesc" "${SHADER_DIR}/*.tese" "${SHADER_DIR}/*.mesh" "${SHADER_DIR}/*.task" "${SHADER_DIR}/*.rgen" "${SHADER_DIR}/*.rchit" "${SHADER_DIR}/*.rmiss" "${SHADER_DIR}/*.rcall")

compile_shader("LearnVKPrecompile" "${SHADER_FILES}" "${SHADER_INCLUDE_DIR}")
```

我们定义了一个编译shader的cmake函数，并将收集的shader源文件传给它：

```cmake
function(compile_shader TARGET_NAME SHADERS SHADER_INCLUDE_DIR)
    set(working_dir "${CMAKE_CURRENT_SOURCE_DIR}")
    set(GLSLANG_BIN $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe) # 设置glslangValidator的位置
    foreach(SHADER ${SHADERS})  # 遍历每一个shader源文件
    get_filename_component(SHADER_NAME ${SHADER} NAME)  # 获取shader的名字
    string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})  # 在生成的.h文件中将文件名的'.'换成'_'
    string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)    # 将存储二进制内容的全局vector对象名改为全部大写
    set(SPV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/spv/${SHADER_NAME}.spv")    # 生成的.spv文件
    set(CPP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/cpp/${HEADER_NAME}.h")      # 生成的.h文件
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLANG_BIN} -I${SHADER_INCLUDE_DIR} -V100 -o ${SPV_FILE} ${SHADER}
        DEPENDS ${SHADER}
        WORKING_DIRECTORY "${working_dir}")             # 添加编译命令，在项目生成时执行
    list(APPEND ALL_GENERATED_SPV_FILES ${SPV_FILE})    
    add_custom_command(
            OUTPUT ${CPP_FILE}
            COMMAND ${CMAKE_COMMAND} -DPATH=${SPV_FILE} -DHEADER="${CPP_FILE}" 
                -DGLOBAL="${GLOBAL_SHADER_VAR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateShaderCPPFile.cmake"
            DEPENDS ${SPV_FILE}
            WORKING_DIRECTORY "${working_dir}")         # 添加执行将spv转换为h文件的cmake函数的命令
    list(APPEND ALL_GENERATED_CPP_FILES ${CPP_FILE})
    endforeach()
    add_custom_target(${TARGET_NAME}    # 将上述过程添加到一个生成目标中
        DEPENDS ${ALL_GENERATED_SPV_FILES} ${ALL_GENERATED_CPP_FILES} SOURCES ${SHADERS})
endfunction()
```

```cmake
# GenerateShaderCPPFile.cmake
function(embed_resource resource_file_name source_file_name variable_name)
    if(EXISTS "${source_file_name}")
        if("${source_file_name}" IS_NEWER_THAN "${resource_file_name}")
            return()
        endif()
    endif()
    if(EXISTS "${resource_file_name}")
        file(READ "${resource_file_name}" hex_content HEX)
        string(REPEAT "[0-9a-f]" 32 pattern)
        string(REGEX REPLACE "(${pattern})" "\\1\n" content "${hex_content}")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " content "${content}")
        string(REGEX REPLACE ", $" "" content "${content}")
        set(array_definition "static const std::vector<unsigned char> ${variable_name} =\n{\n${content}\n};")
        get_filename_component(file_name ${source_file_name} NAME)
        set(source "/**\n * @file ${file_name}\n * @brief Auto generated file.\n */\n#include <vector>\n${array_definition}\n")
        file(WRITE "${source_file_name}" "${source}")
    else()
        message("ERROR: ${resource_file_name} doesn't exist")
        return()
    endif()
endfunction()
if(EXISTS "${PATH}")
    embed_resource("${PATH}" "${HEADER}" "${GLOBAL}")
endif()
```

其中`GenerateShaderCPPFile.cmake`的作用是将生成的.spv文件转为.h代码，并生成一个全局的`std::vector<unsigned char>`对象，让我们在项目中可以直接引用头文件访问，而不需要加载文件。这种实现是参考了GAMES104课程项目Piccolo的着色器生成方式，上述过程用到的代码都可以在其仓库中找到。

通过这种方式就可以在CMake配置工程之后生成一个`LearnVKPrecompile`的自定义工程，生成工程就完成了编译和嵌入式代码的生成。为了让生成的代码可以在项目中使用，还需要添加包含目录：

```cmake
target_include_directories(LearnVK PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generated/cpp)
```

做完上述工作，就可以在代码中通过头文件直接引用编写的着色器代码了。

```c++
#include <vertex_vert.h>	// 着色器的名字叫vertex.vert
#include <fragment_frag.h>
```

### 在管线中使用着色器

获取了字节码之后想要在Vulkan管线中使用就需要包装成着色器模块。

```c++
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

// createPipeline
VkShaderModule vertexShaderModule = createShaderModule(VERTEX_VERT);
VkShaderModule fragmentShaderModule = createShaderModule(FRAGMENT_FRAG);
```

着色器模块也只是封装了着色器代码，我们需要指定这个模块在管线中的什么阶段。通过`VkPipelineShaderStageCreateInfo`创建。

```c++
VkPipelineShaderStageCreateInfo vertStageCreateInfo = {};
vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertStageCreateInfo.pName = "main";
vertStageCreateInfo.module = vertexShaderModule;
vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

VkPipelineShaderStageCreateInfo fragStageCreateInfo = {};
// ...
VkPipelineShaderStageCreateInfo shaderStages[2] = {
	vertStageCreateInfo, fragStageCreateInfo
};
```

至此，着色器在项目中的集成和在图形管线中的设置就已经完成。接下来就是设置其他繁杂的图形管线的固定功能和渲染流程了，这部分内容我将放在下一篇文章中。最后给出管线设置全部实现后，上文给出的着色器代码最终的效果：

![triangle](media\triangle.png)



## 渲染步骤与渲染目标

### RenderPass 渲染步骤

RenderPass这个概念在之前学习Unity Shader的时候就有所接触。Unity中一个shader的某一个SubShader中会有一个或是多个pass。对于每一个pass，都可设置这一个pass的**着色器代码**和**管线固定功能**包括剔除、Blending、ColorMask等等。实际上在Vulkan中，RenderPass(渲染步骤)和Subpass(子步骤)这两者的设计彻底明确了pass的概念，可以让我们窥探所谓游戏渲染中pass设计的一二。

上文中我们提到，每一套ShaderModule需要绑定到一条图形管线才能发挥作用。而在一帧渲染流程中，RenderPass或准确的说**一个subpass对应了一套图形管线状态下的一次渲染**。图形管线的作用就是描述一套pass流程中流程的所有状态。Vulkan中管线叫做`VkGraphicsPipeline`，而DX12中的管线状态对象`PipelineStateObject`的名字某种程度也更好地反应了管线的本质。

在图形管线创建的代码中也有体现，每一个pipeline创建的时候都需要指定一个RenderPass和它的Subpass的索引：

```c++
pipelineCreateInfo.renderPass = m_renderPass;
pipelineCreateInfo.subpass = 0;	// 子流程数组中的索引
```

### Attachment 渲染目标

而Vulkan所谓的Attachment也即DX12的RenderTarget，是一次渲染步骤执行时写入的渲染目标。这个目标可能只需要Color，也有可能需要Depth或是Stencil等等。而在延迟渲染管线中，甚至可能需要多个Attachment来供一个RenderPass输出。

```c++
VkAttachmentDescription colorAttachment = {};
colorAttachment.format = m_swapChainImageFormat;
colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
// 颜色和深度缓冲的存取策略
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
// stencil缓冲的存取策略
colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;	// 在内存中的分布方式为用作呈现方式
```

实际上RenderPass和Attachment的提出解决了移动端的GPU架构使用的硬件渲染器（TBDR）片上缓存的问题。对于TBDR渲染器面对多个pass而言，某些pass的渲染结果是直接被下一个Subpass使用而不需要写回显存在FrameBuffer中展示出来的，因此存储在缓存之中直接被下一个pass读取是最优性能的策略，那么就可以设置上一个pass的attachment的`storeOp`为`VK_ATTACHMENT_STORE_OP_DONT_CARE`；对于某些Subpass不需要考虑之前的pass的结果，那么就可以设置attachment的`loadOp`为`VK_ATTACHMENT_LOAD_OP_DONT_CARE`。

### Subpass子步骤

Subpass相当于一个大车间中的两个流水线。当两个流水线的输入输出能够无缝衔接时，直接把两个流水线放在同一个大车间即可，而不必分别放到两个车间中。每一个Subpass都会有一个Attachment的引用，并且指定指定其布局。这个编号也就是片元着色器中最终指定的输出`layout(location = 0) out vec4 outColor`的location编号。

```c++
// 附着引用
VkAttachmentReference colorAttachReference = {};
colorAttachReference.attachment = 0;	// 表示description在数组中的引用索引，也是shader中片元最终输出时layout (location = 0)的索引
colorAttachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
// 渲染流程可能包含多个子流程，其依赖于上一流程处理后的帧缓冲内容
VkSubpassDescription subpass = {};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// 这是一个图形渲染的子流程
subpass.colorAttachmentCount = 1;
subpass.pColorAttachments = &colorAttachReference;	// 颜色帧缓冲附着会被在frag中使用作为输出
```

### 子流程依赖

上文中提到多个子流程之间存在顺序关系，那么就需要描述他们在执行时的依赖关系。

```c++
// 子流程依赖
VkSubpassDependency dependency = {};
dependency.srcSubpass = VK_SUBPASS_EXTERNAL;//隐含的子流程
dependency.dstSubpass = 0; 
dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 需要等待交换链读取完图像
dependency.srcAccessMask = 0;
dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 为等待颜色附着的输出阶段
dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
```

`srcStageMask`和`srcAccessMask`用于指定需要等待的管线阶段 和子流程将进行的操作类型。我们需要等待交换链结束对图像的读取才能对图像进行访问操作，也就是等待颜色附着输出这一管线阶段。`dstStageMask`和`dstAccessMask`用于指定需要等待的管线阶段和子流程将进行的操作类型。在这里，我们的设置为等待颜色附着的输出阶段，子流程将会进行颜色附着的读写操作。这样设置后，图像布局变换直到必要时才会进行。

### 设置渲染步骤

```c++
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
```



## 图形管线

在理解了图形管线和渲染步骤之间的关系之后，配置图形管线的繁杂的参数反而不是什么困难的问题了。我们遵循一下的配置流程：

```mermaid
flowchart LR
ShaderModule --> VertexInput --> InputAssembly --> viewport --> sccissor --> Rasterization--> MultiSample --> DepthStencil --> ColorBlend --> dynamicState --> pipelineLayout --> renderPass

```

这将是一串又臭又长的配置代码，具体的细节在注释中给出。

```c++
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

// 获取顶点的绑定信息和属性信息
auto bindDesc = Vertex::getBindDescription();
auto attrDesc = Vertex::getAttributeDescriptions();
// 顶点输入
VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
vertexInputCreateInfo.pVertexBindingDescriptions = &bindDesc;
vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
vertexInputCreateInfo.pVertexAttributeDescriptions = attrDesc.data();

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

// 深度与模板测试,这里没有就不设置了
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
```

值得注意的是，虽然对于一套管线在运行时不允许更改状态，但是部分的参数可以通过dynamicState的调整改变管线的设置。比如我们允许了`VK_DYNAMIC_STATE_VIEWPORT``VK_DYNAMIC_STATE_SCISSOR`的改变，那么最终使用`VkCmd_`开头的指令就可以改变对应的状态。如下：

```c++
VkViewport viewport{};
// ***
vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

VkRect2D scissor{};
// ***
vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
```

下表中给出了可以动态更改的一些状态：

```c++
VK_DYNAMIC_STATE_VIEWPORT = 0,
VK_DYNAMIC_STATE_SCISSOR = 1,
VK_DYNAMIC_STATE_LINE_WIDTH = 2,
VK_DYNAMIC_STATE_DEPTH_BIAS = 3,
VK_DYNAMIC_STATE_BLEND_CONSTANTS = 4,
VK_DYNAMIC_STATE_DEPTH_BOUNDS = 5,
VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
VK_DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
VK_DYNAMIC_STATE_STENCIL_REFERENCE = 8,
```

对于某些场景中渲染规则复杂的物体可能需要创建很多的管线，Vulkan还提供了两种方式复用管线，包括Pipeline Cache和Pipeline Derivatization，这些就是后话了。



## FrameBuffer帧缓冲

帧缓冲表示的是一组渲染目标Attachment的集合，引用的是一个swapChain的imageView，因此它也和一个RenderPass紧密关联。因此只需要指定这些元素就可以创建一个帧缓冲。我们为每一个swapChain上的imageView的创建一个帧缓冲。

```c++
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
```

当要指定渲染到某个帧缓冲上，在`VkRenderPassBeginInfo`中指定即可。

```c++
renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[imageIndex];
```



## 指令系统概述

Vulkan几乎所有的GPU执行的功能几乎都由指令的形式驱动完成。代表的函数形式就是`vkCmd_`开头的函数。在这类函数中第一个参数会指定一个`VkCommandBuffer`，它会记录每一次绘制的指令，并且在`vkQueueSubmit`中提交到某一个队列族对应的队列中，交由GPU异步执行。

因此当我们在编写程序使用`vkCmd_`开头的指令时，它并不像OpenGL那样直接执行，而是被指令缓冲录制了下来，在真正提交到队列时被某个GPU线程异步执行。正因为这个特性，Vulkan需要我们手动对GPU端的多线程的同步问题进行管理。

### 指令缓冲与指令池

![指令系统](media\command.png)

在创建一个指令缓冲之前，需要创建指令池来管理缓冲的内存和缓冲对象的分配。因为**每一个指令池中的产生的指令只能被提交到一个队列族的队列**中去，因此需要在初始化时指定图形队列族的索引。

```c++
// createCommandPool
QueueFamiliyIndices indices = findDeviceQueueFamilies(m_physicalDevice);
VkCommandPoolCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
createInfo.queueFamilyIndex = indices.graphicsFamily;
// flags标志位意味指令缓冲对象之间相互独立，不会被一起重置。这在之后多帧并行渲染时会用到。
createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
VkResult res = vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool);
if (res != VK_SUCCESS) {
	throw std::runtime_error("failed to create command pool!");
}
```

有了指令池就可以为一个指令缓冲分配空间了。此处在`commandBuffer`到底需要分配多少个，是否每个帧缓冲都需要对应一个指令缓冲的问题上，无论是中文版还是英文版的教程都比较模糊。英文版源码是只分配了**一个指令缓冲供所有帧缓冲使用**；而中文版是根据`vkAcquireNextImageKHR`获取到了交换链中渲染的下一张图片的索引，**每一个索引对应一个指令缓冲**。

两种写法应该都是可以正常运作，只不过似乎没有必要每一个帧缓冲对应一个command buffer。实际上真正需要多个buffer的原因在于多帧预渲染机制需要单独的指令缓冲，否则就会存在同步的问题。因此我这里给出的创建指令池和指令缓冲的代码均是在多帧预渲染机制下的创建方式。关于多帧预渲染机制很快就会介绍到。

```c++
m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);	// 多帧预渲染的帧数
VkCommandBufferAllocateInfo allocInfo = {};
allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
// 设为顶级缓存，意味着可以直接被提交，但是不能被其他缓存调用
allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
allocInfo.commandPool = m_commandPool;
allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
VkResult res = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
if (res != VK_SUCCESS) {
	throw std::runtime_error("failed to allocate command buffer!");
}
```

`VK_COMMAND_BUFFER_LEVEL_PRIMARY`代表顶级缓存，它可以直接被提交，但是不能被其他缓存调用。还有一种标记`VK_COMMAND_BUFFER_LEVEL_SECONDARY`代表二级缓存，它不能直接提交，但是可以被其他的缓存调用。这样的特性就让我们可以在顶级队列中使用与一些预录的二级指令系列，提升开发效率。



### 记录指令到指令缓冲



### 渲染同步





### 管线输入顶点信息




cmake_minimum_required (VERSION 3.8)
# 设置cmake工具链为vcpkg
if(NOT DEFINED ENV{VCPKG_ROOT_PATH})
	message(FATAL_ERROR "VCPKG_ROOT_PATH not defined!")
endif()
set(VCPKG_PATH $ENV{VCPKG_ROOT_PATH})
set(VCPKG_ROOT ${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake CACHE PATH "")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})

# 项目设置
project ("LearnVK")
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
find_package(tinyobjloader CONFIG REQUIRED)

# 将源代码添加到此项目的可执行文件
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR} "include/*.h")
file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR} "src/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})
add_executable(LearnVK ${SOURCE_FILES})

# 链接到库文件
target_link_libraries(LearnVK PRIVATE glm::glm)
target_link_libraries(LearnVK PRIVATE glfw)
target_link_libraries(LearnVK PRIVATE ${VK_SDK_LIB})
target_link_libraries(LearnVK PRIVATE tinyobjloader::tinyobjloader)


# 添加包含目录
target_include_directories(LearnVK PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(LearnVK PRIVATE ${VK_SDK_INCLUDE})

# 编译着色器
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${SHADER_DIR}/*.vert"
                                                 "${SHADER_DIR}/*.frag"
                                                 "${SHADER_DIR}/*.comp"
                                                 "${SHADER_DIR}/*.geom"
                                                 "${SHADER_DIR}/*.tesc"
                                                 "${SHADER_DIR}/*.tese"
                                                 "${SHADER_DIR}/*.mesh"
                                                 "${SHADER_DIR}/*.task"
                                                 "${SHADER_DIR}/*.rgen"
                                                 "${SHADER_DIR}/*.rchit"
                                                 "${SHADER_DIR}/*.rmiss"
                                                 "${SHADER_DIR}/*.rcall")

include(${CMAKE_SOURCE_DIR}/cmake/compileShaders.cmake)

source_group("shaders" FILES ${SHADER_FILES})
set(SHADER_INCLUDE_DIR ${SHADER_DIR}/include)
compile_shader("LearnVKPrecompile" "${SHADER_FILES}" "${SHADER_INCLUDE_DIR}")

add_dependencies(LearnVK LearnVKPrecompile)

target_include_directories(LearnVK PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generated/cpp)
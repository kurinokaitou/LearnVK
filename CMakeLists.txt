# CMakeList.txt: LearnVK 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
set(VCPKG_ROOT "D:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})

project ("LearnVK")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED ENV{VK_SDK_PATH})
	message(FATAL_ERROR "VK_SDK_PATH not defined!")
endif()
set(VK_SDK_PATH $ENV{VK_SDK_PATH})
set(VK_SDK_INCLUDE ${VK_SDK_PATH}/Include)
set(VK_SDK_LIB ${VK_SDK_PATH}/Lib/vulkan-1.lib)


find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)



# 将源代码添加到此项目的可执行文件。
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR} "include/*.h")
file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR} "src/*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})
add_executable(LearnVK ${SOURCE_FILES})
# add_executable(LearnVK "include/LearnVK.h" "src/LearnVK.cpp")

target_link_libraries(LearnVK PRIVATE glm::glm)
target_link_libraries(LearnVK PRIVATE glfw)
target_link_libraries(LearnVK PRIVATE ${VK_SDK_LIB})

target_include_directories(LearnVK PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(LearnVK PRIVATE ${VK_SDK_INCLUDE})

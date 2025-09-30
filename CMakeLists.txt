cmake_minimum_required(VERSION 3.22)
project(Editor LANGUAGES C CXX)
# Add UTF-8 compile flag for MSVC
if (MSVC)
    add_compile_options(/utf-8)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# All outputs relative to the build folder (CMAKE_BINARY_DIR)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Check For Vulkan
find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install 1.3.275")
endif()

message("Vulkan SDK found ${Vulkan_VERSION}")
message("Vulkan SDK Libs found ${Vulkan_LIBRARIES}")
message("Vulkan SDK Includes found ${Vulkan_INCLUDE_DIR}")
include_directories(${Vulkan_INCLUDE_DIR})

add_subdirectory("Chilli")

include_directories("Chilli/")
include_directories("Chilli/Src/")
include_directories("Chilli/Src/Core/")
include_directories("Chilli/Src/Renderer/")
include_directories("Chilli/Libs/SpdLog/include/")
include_directories("Chilli/Libs/glm/glm/")

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} "Main.cpp")

target_link_libraries(${CMAKE_PROJECT_NAME} ChilliCore ChilliVulkan VulkanMemoryAllocator glm::glm kernel32 user32 glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARIES})

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC CHILLI_ENGINE_DEBUG=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Using Release")
	target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC CHILLI_ENGINE_DEBUG=false)
endif()

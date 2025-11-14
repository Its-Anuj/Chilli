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

set(CHILLI_COMPILE_EXAMPLES OFF)

# Check For Vulkan
find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install 1.3.275")
endif()

message(STATUS "Vulkan SDK: ${VULKAN_SDK_ROOT}")
message(STATUS "Library Dir: ${VULKAN_LIBRARIES_DIR}")
message("Vulkan SDK found ${Vulkan_VERSION}")
message("Vulkan SDK Libs found ${Vulkan_LIBRARIES}")
message("Vulkan SDK Includes found ${Vulkan_INCLUDE_DIR}")

include_directories(${Vulkan_INCLUDE_DIR})

add_subdirectory("Chilli")
add_subdirectory("Extensions")

include_directories("Chilli/")
include_directories("Chilli/Src/")
include_directories("Chilli/Src/Core/")
include_directories("Chilli/Src/Renderer/")
include_directories("Chilli/Libs/SpdLog/include/")
include_directories("Chilli/Libs/glm/glm/")

include_directories("Libs/TinyObjLoader/")

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} "Main.cpp")

target_link_libraries(${CMAKE_PROJECT_NAME} ChilliExtensions ChilliCore ChilliVulkan VulkanMemoryAllocator glm::glm kernel32 user32 glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} ${Vulkan_LIBRARIES})


if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC CHILLI_ENGINE_DEBUG=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Using Release")
	target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC CHILLI_ENGINE_DEBUG=false)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
# -----------------------
# 1️⃣ Prepare Directories
# -----------------------
add_custom_target(PrepareAssetDirs
    COMMAND ${CMAKE_COMMAND} -E echo "Preparing asset directories..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/Prepare_Dirs.py
            ${CMAKE_BINARY_DIR}/bin/Assets/Shaders
            ${CMAKE_BINARY_DIR}/bin/Assets/Textures
            ${CMAKE_BINARY_DIR}/bin/Assets/Models
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating required asset directories"
)
# -----------------------
# 2️⃣ Compile Shaders
# -----------------------
add_custom_target(
    CompileShaders ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/Shader_Compile.py
            ${CMAKE_SOURCE_DIR}/Assets/Shaders/
            ${CMAKE_BINARY_DIR}/bin/Assets/Shaders/
            --glslc "C:/VulkanSDK/1.3.275.0/Bin/glslc.exe"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling all shaders using Python"
)

add_dependencies(CompileShaders PrepareAssetDirs)

# -----------------------
# 3️⃣ Copy Textures
# -----------------------
add_custom_target(
    CopyTextures ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Copying texture assets..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets/Textures
            ${CMAKE_BINARY_DIR}/bin/Assets/Textures
    COMMENT "Copying all textures to binary output directory"
)

add_dependencies(CopyTextures PrepareAssetDirs)

if(${CHILLI_COMPILE_EXAMPLES} MATCHES ON)
    add_subdirectory(Examples)
endif()

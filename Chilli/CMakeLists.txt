file(GLOB_RECURSE CHILLI_CORE_SOURCES Src/Core/*.cpp Src/Core/*/*.cpp Src/Renderer/RenderClasses.cpp Src/Renderer/Renderer.cpp)

# Disable warnings-as-errors for Jolt due to Vulkan SDK issues
if(MSVC)
    string(REPLACE "/WX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories("Src/")
include_directories("Src/Core/")
include_directories("Src/Core/Application/")
include_directories("Src/Core/Window/")
include_directories("Src/Core/Input/")
include_directories("Src/Core/Events/")
include_directories("Src/Core/Layers/")
include_directories("Src/Core/BackBone/")
include_directories("Src/Renderer/")

# Library includes
include_directories("Libs/FastNoise2/include/")
include_directories("Libs/GLFW/include/")
include_directories("Libs/SpdLog/include/")
include_directories("Libs/JoltPhysics/")
include_directories("Libs/VMA/include/")
include_directories("Libs/glm/glm/")
include_directories("Libs/Stb/")
include_directories("Libs/Cgltf/")
include_directories("Libs/UFBX/")
include_directories("Libs/TinyObjLoader/")
include_directories("Libs/SPIRV-Reflect/")

add_subdirectory("Src/Renderer/")
add_subdirectory(Libs/JoltPhysics/Build/ Jolt)
add_subdirectory("Libs/GLFW/")
add_subdirectory("Libs/VMA/")
add_subdirectory("Libs/FastNoise2/")
add_subdirectory("Libs/glm/")

add_library(SPIRV-Reflect STATIC  Libs/SPIRV-Reflect/spirv_reflect.c)
add_library(ufbx STATIC  Libs/UFBX/ufbx.c)

add_library(ChilliCore STATIC ${CHILLI_CORE_SOURCES})
target_link_libraries(ChilliVulkan PRIVATE ${Vulkan_LIBRARIES} VulkanMemoryAllocator )

target_link_libraries(ChilliCore ufbx FastNoise2 Jolt glm::glm kernel32 user32 glfw)

target_precompile_headers(ChilliCore PRIVATE "Src/Core/Ch_PCH.h" )

set(CHILLI_BUILD_JOLT TRUE)

if(CHILLI_LOG MATCHES TRUE)
	message("Can Log")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENABLE_LOG=true)
else()
	message("Cant Log")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENABLE_LOG=false)
endif()

if(CHILLI_BUILD_JOLT MATCHES TRUE)
	message("Build Jolt")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENABLE_JOLT=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Using Release")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=false)
endif()
set(Chilli_EXAMPLE_GRAVITY_COMPILE ON)

include_directories("../")
include_directories("../Chilli/")
include_directories("../Chilli/Src/")
include_directories("../Chilli/Src/Core/")
include_directories("../Chilli/Src/Renderer/")
include_directories("../Chilli/Libs/SpdLog/include/")
include_directories("../Chilli/Libs/glm/glm/")

if(${Chilli_EXAMPLE_GRAVITY_COMPILE} MATCHES ON)
	set(Chilli_EXAMPLE_GRAVITY_NAME "Gravity")

    message("Compiling Gravity")
	add_executable(${Chilli_EXAMPLE_GRAVITY_NAME} "Gravity.cpp")

	target_link_libraries(${Chilli_EXAMPLE_GRAVITY_NAME} ChilliExtensions ChilliCore ChilliVulkan VulkanMemoryAllocator glm::glm kernel32 user32 glfw)
	target_link_libraries(${Chilli_EXAMPLE_GRAVITY_NAME} ${Vulkan_LIBRARIES})

	if(CMAKE_BUILD_TYPE MATCHES "Debug")
		message("Using Debug")
		target_compile_definitions(${Chilli_EXAMPLE_GRAVITY_NAME} PUBLIC CHILLI_ENGINE_DEBUG=true)
	endif()

	if(CMAKE_BUILD_TYPE MATCHES "Release")
		message("Using Release")
		target_compile_definitions(${Chilli_EXAMPLE_GRAVITY_NAME} PUBLIC CHILLI_ENGINE_DEBUG=false)
	endif()
endif()

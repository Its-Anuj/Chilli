file(GLOB_RECURSE CHILLI_VULKAN_SOURCES Vulkan/*.cpp)

include_directories("Src/")
include_directories("Src/Core/")
include_directories("Src/Renderer/")
include_directories(${Vulkan_INCLUDE_DIR})
include_directories("../../../Libs/VMA/include/")
include_directories("../../../Libs/GLFW/include/")

add_library(ChilliVulkan STATIC ${CHILLI_VULKAN_SOURCES})

target_link_libraries(ChilliVulkan PUBLIC ChilliCore)

target_precompile_headers(ChilliVulkan PRIVATE "Vulkan/ChV_PCH.h")

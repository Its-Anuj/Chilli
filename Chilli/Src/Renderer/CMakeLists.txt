file(GLOB_RECURSE CHILLI_VULKAN_SOURCES Vulkan/*.cpp)

include_directories("Src/")
include_directories("Src/Core/")
include_directories("Src/Renderer/")

add_library(ChilliVulkan STATIC ${CHILLI_VULKAN_SOURCES})

target_link_libraries(ChilliVulkan PUBLIC ChilliCore)

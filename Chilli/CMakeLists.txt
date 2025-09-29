file(GLOB_RECURSE CHILLI_CORE_SOURCES Src/Core/*.cpp Src/Core/*/*.cpp Src/Renderer/Renderer.cpp)

include_directories("Src/")
include_directories("Src/Core/")
include_directories("Src/Core/Application/")
include_directories("Src/Core/Window/")
include_directories("Src/Core/Input/")
include_directories("Src/Core/Events/")
include_directories("Src/Core/Layers/")
include_directories("Src/Renderer/")

# Library includes
include_directories("Libs/GLFW/include/")
include_directories("Libs/SpdLog/include/")
include_directories("Libs/VMA/include/")

add_subdirectory("Src/Renderer/")
add_subdirectory("Libs/GLFW/")
add_subdirectory("Libs/VMA/")

add_library(ChilliCore STATIC ${CHILLI_CORE_SOURCES})

target_precompile_headers(ChilliCore PRIVATE "Src/Core/Ch_PCH.h" )

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Using Release")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=false)
endif()
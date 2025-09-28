file(GLOB_RECURSE CHILLI_CORE_SOURCES Src/Core/*/*.cpp Src/Core/*.cpp)

include_directories("Src/")
include_directories("Src/Core/")
include_directories("Src/Renderer/")

# Library includes
include_directories("Libs/GLFW/include/")
include_directories("Libs/SpdLog/include/")

add_subdirectory("Src/Renderer/")
add_subdirectory("Libs/GLFW/")

add_library(ChilliCore STATIC ${CHILLI_CORE_SOURCES} "Src/Core/Application/Application.h")

target_precompile_headers(ChilliCore PRIVATE "Src/Core/PCH/Ch_PCH.h" )

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=true)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release")
	message("Using Release")
	target_compile_definitions(ChilliCore PUBLIC CHILLI_ENGINE_DEBUG=false)
endif()
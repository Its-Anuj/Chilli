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

if(MSVC)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
else()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
endif()

if(WIN32 AND NOT MSVC)
    # We are on Windows but using MinGW (like your msys64 setup)
    add_compile_options(-mavx2 -g) 
    # Remove any manual /Zi, /W4, or /Od calls
endif()

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
else()
    # MinGW/GCC uses -g for debug info, which is already handled in your CXX_FLAGS
    # You don't need a special linker flag here for MinGW
endif()

set(CHILLI_COMPILE_EXAMPLES ON)
set(Chilli_EXAMPLE_GRAVITY_COMPILE ON)
set(CHILLI_LOG TRUE)

# Check For Vulkan
find_package(Vulkan REQUIRED)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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

target_link_libraries(${CMAKE_PROJECT_NAME} ChilliExtensions ChilliCore ChilliVulkan)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	message("Using Debug")
	target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC CHILLI_ENGINE_DEBUG=true)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")

    if(MSVC)
         set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /PROFILE")
    endif()
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
            ${CMAKE_BINARY_DIR}/bin/Assets/Fonts
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
    COMMAND ${CMAKE_COMMAND} -E echo "Copying model assets..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets/Models
            ${CMAKE_BINARY_DIR}/bin/Assets/Models
    COMMENT "Copying all models to binary output directory"
)

# -----------------------
# 3️⃣ Copy Textures
# -----------------------
add_custom_target(
    CopyModels ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Copying texture assets..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets/Textures
            ${CMAKE_BINARY_DIR}/bin/Assets/Textures
    COMMENT "Copying all textures to binary output directory"
)

include(FetchContent)
# 1. Define paths
set(MSDF_ZIP_PATH "${CMAKE_BINARY_DIR}/msdf_tool.zip")
# Fix the path to point to the actual folder, then the file
set(MSDF_TOOL_DIR "${CMAKE_BINARY_DIR}/bin/Tools/msdf")
set(MSDF_EXE_PATH "${MSDF_TOOL_DIR}/msdf-atlas-gen/msdf-atlas-gen.exe")

# 2. Check if the tool already exists to avoid re-downloading
if(NOT EXISTS "${MSDF_EXE_PATH}")
    message(STATUS "--- MSDF Tooling missing. Starting automated setup... ---")

    # Create the tools directory if it doesn't exist
    file(MAKE_DIRECTORY "${MSDF_TOOL_DIR}")

    # 3. Download the official release zip
    # Note: Using 1.2.1 as per your URL, ensure version matches your needs
    message(STATUS "Downloading: https://github.com/Chlumsky/msdf-atlas-gen/releases/download/v1.2.1/msdf-atlas-gen-1.2.1-win64.zip")
    file(DOWNLOAD 
        "https://github.com/Chlumsky/msdf-atlas-gen/releases/download/v1.2.1/msdf-atlas-gen-1.2.1-win64.zip" 
        "${MSDF_ZIP_PATH}"
        SHOW_PROGRESS
        STATUS download_status
    )

    # Check if download succeeded
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download MSDF tool. Error: ${download_status}")
    endif()

    # 4. Unzip the file
    message(STATUS "Extracting MSDF tool...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${MSDF_ZIP_PATH}"
        WORKING_DIRECTORY "${MSDF_TOOL_DIR}"
    )

    # 5. Cleanup the zip file
    file(REMOVE "${MSDF_ZIP_PATH}")
    
    message(STATUS "--- MSDF Tooling ready: ${MSDF_EXE_PATH} ---")
else()
    message(STATUS "--- MSDF Tooling found at ${MSDF_EXE_PATH} ---")
endif()

# 6. Make the path available to your Python script or other build steps
set(MSDF_GEN_EXE "${MSDF_EXE_PATH}" CACHE INTERNAL "Path to MSDF generator")
# 1. Define paths
set(RAW_FONTS_DIR "${CMAKE_SOURCE_DIR}/Assets/RawFonts")
set(PROCESSED_FONTS_DIR "${CMAKE_SOURCE_DIR}/Assets/Fonts")
set(FONT_SCRIPT "${CMAKE_SOURCE_DIR}/Scripts/MsdfGen.py")

# 2. Find Python
find_package(Python3 REQUIRED)

# 3. Add a custom target that runs the script
add_custom_target(ProcessFonts ALL
    COMMAND ${Python3_EXECUTABLE} ${FONT_SCRIPT} 
            ${RAW_FONTS_DIR} 
            ${PROCESSED_FONTS_DIR} 
            ${MSDF_GEN_EXE}
    COMMENT "Checking for new TTF fonts in RawFonts..."
    VERBATIM
)

# 4. Make your main executable depend on the font processing
# This ensures fonts are ready BEFORE the engine tries to load them
add_dependencies(${PROJECT_NAME} ProcessFonts)

# -----------------------
# 3️⃣ Copy Textures
# -----------------------
add_custom_target(
    CopyFonts ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Copying Fonts..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets/Fonts
            ${CMAKE_BINARY_DIR}/bin/Assets/Fonts
    COMMENT "Copying all Fonts to binary output directory"
)

add_dependencies(CopyTextures PrepareAssetDirs)
add_dependencies(CopyModels PrepareAssetDirs)
add_dependencies(CopyFonts PrepareAssetDirs)

if(${CHILLI_COMPILE_EXAMPLES} MATCHES ON)
    add_subdirectory(Examples)
endif()


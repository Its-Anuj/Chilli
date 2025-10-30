file(GLOB_RECURSE CHILLI_EXT_SOURCES *.cpp)

add_library(ChilliExtensions STATIC ${CHILLI_EXT_SOURCES})

include_directories("../")
include_directories("../Chilli/Src/")
include_directories("../Chilli/Src/Core/")

target_link_libraries(ChilliExtensions PUBLIC ChilliCore)

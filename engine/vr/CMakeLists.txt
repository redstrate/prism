set(CMAKE_FOLDER "${CMAKE_FOLDER}/VR")

add_library(VR INTERFACE)
target_include_directories(VR INTERFACE public)

add_custom_target(VRInterface SOURCES
    public/vr.hpp)

if(ENABLE_OPENXR)
    add_subdirectory(openxr)
endif()
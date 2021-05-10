cmake_minimum_required(VERSION 3.20)
project(PrismEngine)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# enable folders in IDEs that support this feature
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/AddPlatformExecutable.cmake)
include(FetchContent)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message("Linux build detected!")

    find_package(SDL2 REQUIRED)

    set(ENABLE_VULKAN TRUE)
    set(ENABLE_LINUX TRUE)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" AND NOT IOS)
    message("macOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_MACOS TRUE)

    set(CMAKE_XCODE_GENERATE_SCHEME OFF)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    message("iOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_IOS TRUE)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "tvOS")
    message("tvOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_TVOS TRUE)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message("Windows build detected!")

    find_package(SDL2 REQUIRED)

    set(ENABLE_WINDOWS ON)
    set(ENABLE_VULKAN ON)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
    message("UWP build detected!")

    set(ENABLE_UWP TRUE)
endif()

set(CROSS_LIBS
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-msl
    glslang
    SPIRV
)

add_subdirectory(extern)

# enable lto
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_subdirectory(platforms)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

source_group(Shaders shaders)

add_subdirectory(engine)
add_subdirectory(tools)
add_subdirectory(example)
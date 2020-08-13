cmake_minimum_required(VERSION 3.17)
project(PrismEngine
    LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# enable folders in IDEs that support this feature
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# enable lto
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Common.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/AddPlatformExecutable.cmake)
include(FetchContent)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" AND NOT IOS)
    message("macOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_MACOS TRUE)

    set(CMAKE_XCODE_GENERATE_SCHEME OFF)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message("Linux build detected!")

    set(ENABLE_VULKAN TRUE)
    set(ENABLE_LINUX TRUE)
endif()

FetchContent_Declare(
  lua
  GIT_REPOSITORY https://github.com/NLua/lua.git
  GIT_TAG master
)

FetchContent_Declare(
  bullet
  GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
  GIT_TAG 2.89
)

FetchContent_Declare(
  spirv-cross
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
  GIT_TAG 2020-05-19
)

FetchContent_Declare(
  glslang
  GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
  GIT_TAG master
)

macro(manual_download)
    set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
    set(BUILD_BULLET3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(INSTALL_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)

    add_definitions(-DLUA_USE_APPLE)

    set(CMAKE_FOLDER "External")

    FetchContent_MakeAvailable(lua)
    FetchContent_MakeAvailable(bullet)
    FetchContent_MakeAvailable(spirv-cross)
    FetchContent_MakeAvailable(glslang)

    remove_definitions(LUA_USE_MACOSX)

    set(CMAKE_FOLDER "")
endmacro()

if(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    message("iOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_IOS TRUE)

    manual_download()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "tvOS")
    message("tvOS build detected!")

    set(ENABLE_METAL TRUE)
    set(ENABLE_DARWIN TRUE)
    set(ENABLE_TVOS TRUE)

    manual_download()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message("Windows build detected!")

    set(ENABLE_WINDOWS ON)
    set(ENABLE_VULKAN ON)

    manual_download()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
    message("UWP build detected!")

    set(ENABLE_UWP TRUE)

    manual_download()
endif()

if(NOT ENABLE_IOS AND NOT ENABLE_TVOS AND NOT ENABLE_WINDOWS)
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_cpp REQUIRED)
    find_package(spirv_cross_msl REQUIRED)
    find_package(glslang REQUIRED)

    set(CROSS_LIBS
        glslang::glslang
        glslang::SPIRV
        glslang::OSDependent
        glslang::OGLCompiler
        glslang::HLSL
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-cpp
        spirv-cross-msl
    )
else()
    set(CROSS_LIBS
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-cpp
        spirv-cross-msl
        glslang
        SPIRV
    )
endif()


add_subdirectory(extern)
add_subdirectory(platforms)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(engine)

set(CMAKE_FOLDER "Tools" PARENT_SCOPE)

if(NOT IOS AND NOT ENABLE_TVOS)
    add_subdirectory(tools/shadercompiler)
endif()

if(BUILD_TOOLS)
    add_subdirectory(tools/common)
    add_subdirectory(tools/fontcompiler)
    add_subdirectory(tools/editor)
    add_subdirectory(tools/modelcompiler)
    add_subdirectory(tools/cutsceneeditor)
endif()

set(CMAKE_FOLDER "" PARENT_SCOPE)

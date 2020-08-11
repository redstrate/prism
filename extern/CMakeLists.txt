set(CMAKE_FOLDER "External")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

else()
	add_definitions(-Wno-everything)
endif()

set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

add_subdirectory(stb)
add_subdirectory(imgui)
add_subdirectory(magic_enum)
add_subdirectory(smaa)
add_subdirectory(sol)
add_subdirectory(doctest)

include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git
  GIT_TAG v3.9.1)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(NOT ENABLE_IOS AND NOT ENABLE_TVOS)
add_subdirectory(ogg)
add_subdirectory(opus)

add_library(opusfile
	opusfile/src/opusfile.c
	opusfile/src/info.c
	opusfile/src/internal.c
	opusfile/src/stream.c)
target_include_directories(opusfile
	PUBLIC
	opusfile/include)
target_link_libraries(opusfile
	PUBLIC
	ogg
	opus)

	add_subdirectory(portaudio)
endif()

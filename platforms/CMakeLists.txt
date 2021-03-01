if(ENABLE_WINDOWS)
	add_subdirectory(windows)
endif()

if(ENABLE_LINUX)
	add_subdirectory(sdl)
endif()

if(ENABLE_MACOS)
	add_subdirectory(mac)
endif()

if(ENABLE_IOS)
	add_subdirectory(ios)
endif()

if(ENABLE_TVOS)
	add_subdirectory(tvos)
endif()
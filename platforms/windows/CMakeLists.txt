include(../../cmake/AddPlatformExecutable.cmake)

add_library(WindowsSupport
	windows.cpp)
target_link_libraries(WindowsSupport PRIVATE Platform)

add_platform(
    MAIN_FILE
    main.cpp.in
    EXECUTABLE_PROPERTIES
    LINK_LIBRARIES
    Core
    GFXVulkan
    FileWindows
    LangWindows
    WindowsSupport
    GFXDummy
    GFXOpenGL
    Platform
    COMPILE_OPTIONS
)

add_custom_target(PlatformWindows_IDE SOURCES
    main.cpp.in)

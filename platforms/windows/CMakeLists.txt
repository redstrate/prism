include(../../cmake/AddPlatformExecutable.cmake)

add_platform(
    MAIN_FILE
    main.cpp.in
    SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/windows.cpp
    EXECUTABLE_PROPERTIES
    LINK_LIBRARIES
    Core
    GFXVulkan
)

add_custom_target(PlatformWindows_IDE SOURCES
    main.cpp.in)

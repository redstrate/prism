set(CMAKE_FOLDER "Tools")

if(NOT IOS AND NOT ENABLE_TVOS)
    add_subdirectory(shadercompiler)
endif()

if(BUILD_TOOLS)
    add_subdirectory(common)
    add_subdirectory(fontcompiler)
    add_subdirectory(editor)
    add_subdirectory(modelcompiler)
    add_subdirectory(cutsceneeditor)
endif()

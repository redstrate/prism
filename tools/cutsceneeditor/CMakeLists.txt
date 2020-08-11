add_platform_executable(
    TARGET CutsceneEditor
    APP_CLASS CutsceneEditor
    APP_INCLUDE cutsceneeditor.hpp
    SKIP_DATA ON
    SRC
    src/cutsceneeditor.cpp
    include/cutsceneeditor.hpp)
target_link_libraries(CutsceneEditor PUBLIC
    Core
    EditorCommon)
target_include_directories(CutsceneEditor PUBLIC
    include)
set_engine_properties(CutsceneEditor)
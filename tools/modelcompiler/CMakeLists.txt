add_platform_executable(
    TARGET ModelCompiler
    APP_CLASS ModelEditor
    APP_INCLUDE modeleditor.hpp
    SRC
    src/modeleditor.cpp
    include/modeleditor.hpp)
target_link_libraries(ModelCompiler PUBLIC
    Core
    EditorCommon
    PRIVATE
    assimp)
target_include_directories(ModelCompiler PUBLIC
    include)
set_engine_properties(ModelCompiler)

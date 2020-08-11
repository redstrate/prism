find_package(assimp REQUIRED)

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
    ${ASSIMP_LIBRARIES})
target_include_directories(ModelCompiler PUBLIC
    include
    PRIVATE
    ${ASSIMP_INCLUDE_DIRS})
add_dependencies(ModelCompiler BuildShaders)
set_engine_properties(ModelCompiler)

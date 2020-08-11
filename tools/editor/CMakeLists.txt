set(SRC
    include/prismeditor.hpp
    include/materialeditor.hpp
    include/sceneeditor.hpp
    include/uieditor.hpp
    include/prefabeditor.hpp
    
    src/prismeditor.cpp
    src/materialeditor.cpp
    src/sceneeditor.cpp
    src/uieditor.cpp
    src/prefabeditor.cpp)

add_platform_executable(
    TARGET PrismEditor
    APP_CLASS PrismEditor
    APP_INCLUDE prismeditor.hpp
    SKIP_DATA ON
    SRC ${SRC})
target_link_libraries(PrismEditor PUBLIC
    Core
    EditorCommon)
target_include_directories(PrismEditor PUBLIC
    include)
set_engine_properties(PrismEditor)
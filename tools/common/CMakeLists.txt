set(SRC
	include/commoneditor.hpp
	include/debugpass.hpp
    include/undostack.hpp
    
	src/commoneditor.cpp
	src/debugpass.cpp
    src/undostack.cpp)

add_library(EditorCommon ${SRC})
target_include_directories(EditorCommon PUBLIC include)
target_link_libraries(EditorCommon PUBLIC
	Renderer
	imgui
	Core)
set_engine_properties(EditorCommon)

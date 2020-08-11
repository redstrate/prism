add_library(EditorCommon
	include/commoneditor.hpp
	include/debugpass.hpp
    include/undostack.hpp
    
	src/commoneditor.cpp
	src/debugpass.cpp
    src/undostack.cpp)
target_include_directories(EditorCommon PUBLIC include)
target_link_libraries(EditorCommon PUBLIC
	Renderer
	imgui
	Core)
set_target_properties(EditorCommon PROPERTIES
    CXX_STANDARD 17)

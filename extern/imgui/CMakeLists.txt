add_library(imgui STATIC
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_widgets.cpp
    src/imgui.cpp
    src/imgui_stdlib.cpp
    include/imconfig.h
    include/imgui_stdlib.h
    include/imgui.h
    src/imgui_internal.h
    src/imgui_tables.cpp)
target_include_directories(imgui PUBLIC include src)
target_link_libraries(imgui PUBLIC stb Utility)

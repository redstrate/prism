set(SRC
    include/renderer.hpp
    include/font.hpp
    include/pass.hpp
    include/shadowpass.hpp
    include/imguipass.hpp
    include/smaapass.hpp
    include/scenecapture.hpp
    include/materialcompiler.hpp
    include/dofpass.hpp
    include/frustum.hpp
    include/render_options.hpp
    include/rendertarget.hpp

    src/renderer.cpp
    src/shadowpass.cpp
    src/imguipass.cpp
    src/smaapass.cpp
    src/scenecapture.cpp
    src/materialcompiler.cpp
    src/dofpass.cpp
    src/frustum.cpp)

add_library(Renderer STATIC ${SRC})
target_link_libraries(Renderer
    PUBLIC
    GFX
    Math
    PRIVATE
    stb
    Math
    Utility
    imgui
    SMAA::SMAA
    ShaderCompiler
    Core)
target_include_directories(Renderer PUBLIC include)
set_engine_properties(Renderer)
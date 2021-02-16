set(SRC
    include/engine.hpp
    include/app.hpp
    include/input.hpp
    include/cutscene.hpp
    include/physics.hpp
    include/scene.hpp
    include/imguilayer.hpp
    include/uielement.hpp
    include/screen.hpp
    include/object.hpp
    include/debug.hpp
    include/components.hpp
    include/imgui_utility.hpp
    include/console.hpp

    src/file.cpp
    src/engine.cpp
    src/input.cpp
    src/physics.cpp
    src/imguilayer.cpp
    src/screen.cpp
    src/scene.cpp
    src/debug.cpp
    src/console.cpp)

if(NOT ENABLE_IOS AND NOT ENABLE_TVOS)
    set(EXTRA_LIBRARIES Audio)
endif()

add_library(Core STATIC ${SRC})
target_include_directories(Core PUBLIC
    include)
target_include_directories(Core SYSTEM PUBLIC
    ${CMAKE_BINARY_DIR}/_deps/bullet-src/src)
target_link_libraries(Core PUBLIC
    Asset
    Math
    Utility
    nlohmann_json
    Log
    PRIVATE
    GFX
    Renderer
    magic_enum
    imgui
    Platform
    ${EXTRA_LIBRARIES}
    BulletDynamics
    BulletCollision
    LinearMath)
set_engine_properties(Core)

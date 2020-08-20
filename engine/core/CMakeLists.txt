if(TARGET BulletDynamics)
    set(BULLET_LIBRARIES BulletDynamics BulletCollision LinearMath)
    set(BULLET_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/_deps/bullet-src/src)
else()
    find_package(Bullet REQUIRED)
endif()

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
    include/platform.hpp
    include/file.hpp
    include/imgui_utility.hpp

    src/file.cpp
    src/engine.cpp
    src/input.cpp
    src/physics.cpp
    src/imguilayer.cpp
    src/screen.cpp
    src/scene.cpp
    src/debug.cpp)

if(NOT ENABLE_IOS AND NOT ENABLE_TVOS)
    set(EXTRA_LIBRARIES Audio)
endif()

add_library(Core STATIC ${SRC})
target_include_directories(Core PUBLIC
    include)
target_include_directories(Core SYSTEM PUBLIC
    ${BULLET_INCLUDE_DIRS})
target_link_libraries(Core PUBLIC
    Utility
    GFX
    Renderer
    nlohmann_json
    magic_enum
    ${BULLET_LIBRARIES}
    imgui
    Log
    ${EXTRA_LIBRARIES}
    Asset)
set_engine_properties(Core)

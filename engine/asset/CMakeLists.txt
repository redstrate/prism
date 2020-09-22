set(SRC
    include/asset_types.hpp
    include/asset.hpp
    include/assetptr.hpp
    include/material_nodes.hpp
    
    src/asset.cpp)

add_library(Asset STATIC ${SRC})
target_include_directories(Asset PUBLIC include)
target_link_libraries(Asset
    PUBLIC
    Math
    Renderer
    PRIVATE
    stb
    Log
    Core)
set_engine_properties(Asset)

add_library(GFXDummy STATIC src/gfx_dummy.cpp)
target_include_directories(GFXDummy PUBLIC include)
target_link_libraries(GFXDummy PUBLIC GFX)
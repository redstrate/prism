set(SRC
    include/audio.hpp
    src/audio.cpp)

add_library(Audio STATIC ${SRC})
target_include_directories(Audio PUBLIC include)
target_link_libraries(Audio PRIVATE
    Log
    opus
    opusfile
    portaudio_static
    Core)
set_engine_properties(Audio)

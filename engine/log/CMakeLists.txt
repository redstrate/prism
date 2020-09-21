set(SRC
    include/log.hpp
    src/log.cpp)

add_library(Log STATIC ${SRC})
target_include_directories(Log PUBLIC include)
target_link_libraries(Log PRIVATE Utility)
set_engine_properties(Log)

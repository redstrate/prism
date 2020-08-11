set(SRC
    include/log.hpp
    src/log.cpp)

add_library(Log STATIC ${SRC})
target_include_directories(Log PUBLIC include)
target_compile_features(Log PUBLIC cxx_std_17)
set_target_properties(Log PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(Log PRIVATE Utility)

set(SRC
    include/utility.hpp
    include/json_conversions.hpp
    include/string_utils.hpp
    include/timer.hpp
    include/common.hpp
    include/aabb.hpp
    
    src/string_utils.cpp)

add_library(Utility ${SRC})
target_link_libraries(Utility PUBLIC Math magic_enum)
target_include_directories(Utility PUBLIC include)

set(SRC
    include/math.hpp
    include/matrix.hpp
    include/transform.hpp
    include/vector.hpp
    include/quaternion.hpp
    include/plane.hpp
    include/aabb.hpp

    src/transform.cpp
    src/math.cpp include/ray.hpp)

add_library(Math STATIC ${SRC})
target_include_directories(Math PUBLIC include)
set_engine_properties(Math)

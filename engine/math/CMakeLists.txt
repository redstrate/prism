set(SRC
    include/math.hpp
    include/matrix.hpp
    include/transform.hpp
    include/vector.hpp
    include/quaternion.hpp
    include/plane.hpp

    src/transform.cpp
    src/math.cpp)

add_library(Math STATIC ${SRC})
target_include_directories(Math PUBLIC include)
target_compile_features(Math PUBLIC cxx_std_17)
set_target_properties(Math PROPERTIES CXX_EXTENSIONS OFF)

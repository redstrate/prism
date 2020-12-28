set(SRC
    include/example.hpp
    src/example.cpp)

add_platform_executable(
    TARGET ExampleApp
    APP_CLASS ExampleApp
    APP_INCLUDE example.hpp
    SRC ${SRC})
target_link_libraries(ExampleApp PUBLIC
    Core)
target_include_directories(ExampleApp PUBLIC
    include)
set_engine_properties(ExampleApp)
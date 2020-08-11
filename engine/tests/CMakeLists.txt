add_executable(Tests 
    tests.cpp
    string_tests.cpp
    utility_tests.cpp)
target_link_libraries(Tests PUBLIC doctest Utility)
set_output_dir(Tests)
set_engine_properties(Tests)

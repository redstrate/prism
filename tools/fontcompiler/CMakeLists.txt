add_executable(FontCompiler main.cpp)
target_link_libraries(FontCompiler PRIVATE stb)
set_engine_properties(FontCompiler)
set_output_dir(FontCompiler)

add_executable(ShaderCompiler main.cpp)
target_link_libraries(ShaderCompiler
    PUBLIC
    ${CROSS_LIBS}
	Log)
set_engine_properties(ShaderCompiler)
set_output_dir(ShaderCompiler)

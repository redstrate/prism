add_executable(ShaderCompilerTool main.cpp)
target_link_libraries(ShaderCompilerTool
    PRIVATE
    ShaderCompiler
	Log
    Utility)
set_engine_properties(ShaderCompilerTool)
set_output_dir(ShaderCompilerTool)

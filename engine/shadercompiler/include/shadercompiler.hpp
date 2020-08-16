#pragma once

#include <variant>
#include <string>
#include <vector>
#include <string_view>
#include <optional>

/// The shader stage that the shader is written in.
enum class ShaderStage {
    Vertex,
    Fragment
};

/// The shader language that the shader is written in.
enum class ShaderLanguage {
    GLSL,
    MSL,
    SPIRV
};

/// Compilation options when compiling shaders.
class CompileOptions {
public:
    /// Adds a GLSL define.
    void add_definition(const std::string_view name) {
        definitions.emplace_back(name);
    }
    
    std::vector<std::string> definitions;
    
    /// When compiling MSL, the result may differ whether or not we're targetting non-Mac Metal platforms.
    bool is_apple_mobile = false;
};

/// Represents the source code of a shader either in plaintext (GLSL, MSL) or bytecode (SPIR-V).
class ShaderSource {
public:
    ShaderSource() {}
    ShaderSource(const ShaderSource& rhs) : source (rhs.source) {}
    ShaderSource(const std::string source_string) : source(source_string) {}
    ShaderSource(const std::vector<uint32_t> source_bytecode) : source(source_bytecode) {}
        
    std::variant<std::string, std::vector<uint32_t>> source;
    
    /// Returns a view of the shader source as plaintext.
    std::string_view as_string() const {
        return std::get<std::string>(source);
    }
    
    /// Returns a copy of the shader source as bytecode.
    std::vector<uint32_t> as_bytecode() const {
        return std::get<std::vector<uint32_t>>(source);
    }
};

/// Compiles GLSL shaders to a specified shader language offline or at runtime.
class ShaderCompiler {
public:
    ShaderCompiler();
    
    /// Sets the include directory used to search for files inside of #include directives.
    void set_include_path(const std::string_view path);
    
    /**
     Compiles from one shader language to another shader language.
     @param from_language The language the shader passed by shader_source is written in.
     @param shader_stage The shader stage of the shader.
     @param shader_source The shader code.
     @param to_language The shader language to compile to.
     @param options Optional compilation parameters.
     @note Right now, only GLSL is supported as a source shader language.
     */
    std::optional<ShaderSource> compile(const ShaderLanguage from_language, const ShaderStage shader_stage, const ShaderSource& shader_source, const ShaderLanguage to_language, const CompileOptions& options = CompileOptions());
};

static inline ShaderCompiler shader_compiler;

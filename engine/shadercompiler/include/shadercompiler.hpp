#pragma once

#include <variant>
#include <string>
#include <vector>
#include <string_view>
#include <optional>

enum class ShaderStage {
    Vertex,
    Fragment
};

enum class ShaderLanguage {
    GLSL,
    MSL,
    SPIRV
};

class CompileOptions {
public:
    void add_definition(const std::string_view name) {
        definitions.emplace_back(name);
    }
    
    std::vector<std::string> definitions;
    
    bool is_apple_mobile = false;
};

class ShaderSource {
public:
    ShaderSource(const std::string source_string) : source(source_string) {}
    ShaderSource(const std::vector<uint32_t> source_bytecode) : source(source_bytecode) {}
        
    std::variant<std::string, std::vector<uint32_t>> source;
    
    std::string_view as_string() const {
        return std::get<std::string>(source);
    }
    
    std::vector<uint32_t> as_bytecode() const {
        return std::get<std::vector<uint32_t>>(source);
    }
};

class ShaderCompiler {
public:
    ShaderCompiler();
    
    void set_include_path(const std::string_view path);
    
    /**
     Compiles from one shader language to another shader language.
     */
    std::optional<ShaderSource> compile(const ShaderLanguage from_language, const ShaderStage shader_stage, const ShaderSource& shader_source, const ShaderLanguage to_language, const CompileOptions& options = CompileOptions());
};

static inline ShaderCompiler shader_compiler;

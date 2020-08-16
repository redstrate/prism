#include "shadercompiler.hpp"

#include <spirv_cpp.hpp>
#include <spirv_msl.hpp>
#include <SPIRV/GlslangToSpv.h>

#include "log.hpp"
#include "string_utils.hpp"
#include "includer.hpp"
#include "defaultresources.hpp"

static inline std::string include_path;

ShaderCompiler::ShaderCompiler() {
    glslang::InitializeProcess();
}

void ShaderCompiler::set_include_path(const std::string_view path) {
    include_path = path;
}

const std::vector<uint32_t> compile_glsl_to_spv(const std::string_view source_string, const EShLanguage shader_language, const CompileOptions& options) {
    std::string newString = "#version 430 core\n";
    
    newString += "#extension GL_GOOGLE_include_directive : enable\n";
    newString += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    
    for(auto& definition : options.definitions)
        newString += "#define " + definition + "\n";
    
    newString += "#line 1\n";
    
    newString += source_string;
    
    const char* InputCString = newString.c_str();
    
    glslang::TShader Shader(shader_language);
    
    Shader.setStrings(&InputCString, 1);
    
    int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_1;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;
    
    Shader.setEnvInput(glslang::EShSourceGlsl, shader_language, glslang::EShClientVulkan, ClientInputSemanticsVersion);
    Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);
    
    TBuiltInResource Resources = DefaultTBuiltInResource;
    EShMessages messages = (EShMessages) (EShMsgSpvRules);
    
    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory(include_path);

    if (!Shader.parse(&Resources, 100, false, (EShMessages)0, includer)) {
        console::error(System::Renderer, "{}", Shader.getInfoLog());
        
        return {};
    }
    
    glslang::TProgram Program;
    Program.addShader(&Shader);
    
    if(!Program.link(messages)) {
        console::error(System::None, "Failed to link shader: {} {} {}", source_string.data(), Shader.getInfoLog(), Shader.getInfoDebugLog());
        
        return {};
    }
    
    std::vector<unsigned int> SpirV;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*Program.getIntermediate(shader_language), SpirV, &logger, &spvOptions);
    
    return SpirV;
}

std::optional<ShaderSource> ShaderCompiler::compile(const ShaderLanguage from_language, const ShaderStage shader_stage, const ShaderSource& shader_source, const ShaderLanguage to_language, const CompileOptions& options) {
    if(from_language != ShaderLanguage::GLSL) {
        console::error(System::Renderer, "Non-supported input language!");
        return std::nullopt;
    }
    
    EShLanguage lang = EShLangMiss;
    switch(shader_stage) {
        case ShaderStage::Vertex:
            lang = EShLangVertex;
            break;
        case ShaderStage::Fragment:
            lang = EShLangFragment;
            break;
    }

    auto spirv = compile_glsl_to_spv(shader_source.as_string(), lang, options);
    if(spirv.empty()) {
        console::error(System::Renderer, "SPIRV generation failed!");
        return std::nullopt;
    }
    
    switch(to_language) {
        case ShaderLanguage::MSL:
        {
            spirv_cross::CompilerMSL msl(std::move(spirv));
            
            spirv_cross::CompilerMSL::Options opts;
            if(options.is_apple_mobile) {
                opts.platform = spirv_cross::CompilerMSL::Options::Platform::macOS;
            } else {
                opts.platform = spirv_cross::CompilerMSL::Options::Platform::iOS;
            }
            opts.enable_decoration_binding = true;
            
            msl.set_msl_options(opts);
            
            return msl.compile();
        }
        case ShaderLanguage::SPIRV:
            return spirv;
        default:
            return {};
    }
}

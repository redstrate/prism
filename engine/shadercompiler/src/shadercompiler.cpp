#include "shadercompiler.hpp"

#include <spirv_cpp.hpp>
#include <spirv_msl.hpp>
#include <SPIRV/GlslangToSpv.h>

#include "log.hpp"
#include "string_utils.hpp"
#include "includer.hpp"
#include "defaultresources.hpp"

static inline std::vector<std::string> include_path;

ShaderCompiler::ShaderCompiler() {
    glslang::InitializeProcess();
}

void ShaderCompiler::set_include_path(const std::string_view path) {
    include_path.emplace_back(path.data());
}

std::vector<uint32_t> compile_glsl_to_spv(const std::string_view source_string, const EShLanguage shader_language, const CompileOptions& options) {
    std::string newString = "#version 460 core\n";
    
    newString += "#extension GL_GOOGLE_include_directive : enable\n";
    newString += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    
    for(auto& definition : options.definitions)
        newString += "#define " + definition + "\n";
    
    newString += "#line 1\n";
    
    newString += source_string;
    
    const char* InputCString = newString.c_str();
    
    glslang::TShader shader(shader_language);
    shader.setStrings(&InputCString, 1);
    
    int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100

    shader.setEnvInput(glslang::EShSourceGlsl,
                       shader_language,
                       glslang::EShClientVulkan,
                       ClientInputSemanticsVersion);

    // we are targeting vulkan 1.1, so that uses SPIR-V 1.3
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv,  glslang::EShTargetSpv_1_3);

    DirStackFileIncluder file_includer;
    for(const auto& path : include_path)
        file_includer.pushExternalLocalDirectory(path);

    if (!shader.parse(&DefaultTBuiltInResource, 100, false, EShMsgDefault, file_includer)) {
        prism::log::error(System::Renderer, "{}", shader.getInfoLog());
        
        return {};
    }
    
    glslang::TProgram Program;
    Program.addShader(&shader);
    
    if(!Program.link(EShMsgDefault)) {
        prism::log::error(System::None, "Failed to link shader: {} {} {}", source_string.data(), shader.getInfoLog(), shader.getInfoDebugLog());
        
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
        prism::log::error(System::Renderer, "Non-supported input language!");
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
        case ShaderStage::Compute:
            lang = EShLangCompute;
            break;
    }

    auto spirv = compile_glsl_to_spv(shader_source.as_string(), lang, options);
    if(spirv.empty()) {
        prism::log::error(System::Renderer, "SPIRV generation failed!");
        return std::nullopt;
    }
    
    switch(to_language) {
        case ShaderLanguage::MSL:
        {
            spirv_cross::CompilerMSL msl(std::move(spirv));
            
            spirv_cross::CompilerMSL::Options opts;
            if(options.is_apple_mobile) {
                opts.platform = spirv_cross::CompilerMSL::Options::Platform::iOS;
            } else {
                opts.platform = spirv_cross::CompilerMSL::Options::Platform::macOS;
            }
            opts.enable_decoration_binding = true;
            
            msl.set_msl_options(opts);
            
            return ShaderSource(msl.compile());
        }
        case ShaderLanguage::SPIRV:
            return ShaderSource(spirv);
        default:
            return {};
    }
}

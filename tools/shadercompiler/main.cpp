#include <fstream>
#include <sstream>

#include <spirv_cross/spirv_cpp.hpp>
#include <spirv_cross/spirv_msl.hpp>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

#include "DirStackIncluder.h"
#include "log.hpp"

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

const std::vector<unsigned int> CompileGLSL(const std::string& filename, EShLanguage ShaderType) {
    std::string newString = "#version 430 core\n";

    newString += "#extension GL_GOOGLE_include_directive : enable\n";
    newString += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    
    newString += "#line 1\n";
    
	newString += filename;

    const char* InputCString = newString.c_str();

    glslang::TShader Shader(ShaderType);

    Shader.setStrings(&InputCString, 1);

    int ClientInputSemanticsVersion = 100; // maps to, say, #define VULKAN 100
    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_1;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;

    Shader.setEnvInput(glslang::EShSourceGlsl, ShaderType, glslang::EShClientVulkan, ClientInputSemanticsVersion);
    Shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    Shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    TBuiltInResource Resources = DefaultTBuiltInResource;
    EShMessages messages = (EShMessages) (EShMsgSpvRules);

    DirStackFileIncluder includer;

    if (!Shader.parse(&Resources, 100, false, messages, includer)) {
        console::error(System::None, "Failed to parse shader: {} {} {}", filename, Shader.getInfoLog(), Shader.getInfoDebugLog());

        return {};
    }

    glslang::TProgram Program;
    Program.addShader(&Shader);

    if(!Program.link(messages)) {
        console::error(System::None, "Failed to link shader: {} {} {}", filename, Shader.getInfoLog(), Shader.getInfoDebugLog());

        return {};
    }

    std::vector<unsigned int> SpirV;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*Program.getIntermediate(ShaderType), SpirV, &logger, &spvOptions);

    return SpirV;
}

int main(int argc, char* argv[]) {
    if(argc < 2)
        return -1;
    
    glslang::InitializeProcess();

    std::ifstream t(argv[1]);
    std::stringstream buffer;
    buffer << t.rdbuf();
    
    if(std::string(argv[1]).find("nocompile") != std::string::npos) {
        std::string outname = argv[2];
        outname = outname.substr(0, outname.length() - 5); // remove .glsl
        outname = outname.substr(0, outname.length() - 10); // remove .nocompile
        
        std::ofstream out(outname + ".glsl");
        out << buffer.rdbuf();
    } else {
        EShLanguage lang;
        if (std::string(argv[1]).find("vert") != std::string::npos) {
            lang = EShLangVertex;
        }
        else {
            lang = EShLangFragment;
        }

        std::string outname = argv[2];
        outname = outname.substr(0, outname.length() - 5);

        // generate msl
    #ifdef PLATFORM_MACOS
        bool is_ios = (bool)argv[3];

        {
            auto vertexSPIRV = CompileGLSL(buffer.str(), lang);
            if (vertexSPIRV.size() == 0)
                return -1;

            spirv_cross::CompilerMSL msl(std::move(vertexSPIRV));

            spirv_cross::CompilerMSL::Options opts;
            opts.platform = is_ios ? spirv_cross::CompilerMSL::Options::Platform::iOS : spirv_cross::CompilerMSL::Options::Platform::macOS;
            opts.enable_decoration_binding = true;

            msl.set_msl_options(opts);

            std::string s = msl.compile();

            std::ofstream out(outname + ".msl"); // remove .glsl
            out << s;
        }
    #else
        // generate spv
        {
            auto vertexSPIRV = CompileGLSL(buffer.str(), lang);
            if (vertexSPIRV.size() == 0)
                return -1;

            std::ofstream out(outname + ".spv", std::ios::binary); // remove .glsl
            out.write((char*)vertexSPIRV.data(), vertexSPIRV.size() * sizeof(uint32_t));
        }
    #endif
    }

    return 0;
}

#include <fstream>
#include <sstream>

#include "shadercompiler.hpp"
#include "log.hpp"
#include "string_utils.hpp"

bool has_extension(const std::filesystem::path path, const std::string_view extension) {
    return string_contains(path.filename().string(), extension);
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        console::error(System::Core, "Not enough arguments!");
        return -1;
    }
    
    shader_compiler.set_include_path(std::filesystem::current_path().string());
    
    std::filesystem::path source_path = argv[1];
    std::filesystem::path destination_path = argv[2];

    std::ifstream t(source_path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    
    if(has_extension(source_path, "nocompile")) {
        std::string outname = argv[2];
        outname = outname.substr(0, outname.length() - 5); // remove .glsl
        outname = outname.substr(0, outname.length() - 10); // remove .nocompile
        
        std::ofstream out(outname + ".glsl");
        out << buffer.rdbuf();
        
        console::info(System::Core, "Successfully written {} to {}.", source_path, destination_path);

        return 0;
    } else {
        ShaderStage stage;
        if(has_extension(source_path, ".vert"))
            stage = ShaderStage::Vertex;
        else
            stage = ShaderStage::Fragment;

        ShaderLanguage language;
        CompileOptions options;
#ifdef PLATFORM_MACOS
        options.is_apple_mobile = (bool)argv[3];

        language = ShaderLanguage::MSL;
        destination_path.replace_extension(".msl");
#else
        language = ShaderLanguage::SPIRV;
        destination_path.replace_extension(".spv");
#endif
        
        const auto compiled_source = shader_compiler.compile(ShaderLanguage::GLSL, stage, buffer.str(), language, options);
        if(!compiled_source.has_value()) {
            console::error(System::Core, "Error when compiling {}!", source_path);
            return -1;
        }
        
        switch(language) {
            case ShaderLanguage::SPIRV:
            {
                const auto spirv = compiled_source->as_bytecode();
                
                std::ofstream out(destination_path, std::ios::binary); // remove .glsl
                out.write((char*)spirv.data(), spirv.size() * sizeof(uint32_t));
            }
                break;
            case ShaderLanguage::MSL:
            {
                std::ofstream out(destination_path); // remove .glsl
                out << compiled_source->as_string();
            }
                break;
            default:
                break;
        }
        
        console::info(System::Core, "Successfully written shader from {} to {}.", source_path, destination_path);
        
        return 0;
    }
}

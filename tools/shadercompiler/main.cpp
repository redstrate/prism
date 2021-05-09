#include <fstream>
#include <sstream>
#include <filesystem>

#include "shadercompiler.hpp"
#include "log.hpp"
#include "string_utils.hpp"
#include "file_utils.hpp"

bool has_extension(const std::filesystem::path path, const std::string_view extension) {
    return string_contains(path.filename().string(), extension);
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        prism::log::error(System::Core, "Not enough arguments!");
        return -1;
    }
    
    shader_compiler.set_include_path(std::filesystem::current_path().string());
    
    std::filesystem::path source_path = argv[1];
    std::filesystem::path destination_path = argv[2];

    std::ifstream t(source_path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    
    if(has_extension(source_path, "nocompile")) {
        destination_path = remove_substring(destination_path.string(), ".nocompile"); // remove extension

        std::ofstream out(destination_path);
        out << buffer.rdbuf();
    } else {
        ShaderStage stage = ShaderStage::Vertex;
        if(has_extension(source_path, ".vert"))
            stage = ShaderStage::Vertex;
        else if(has_extension(source_path, ".frag"))
            stage = ShaderStage::Fragment;
        else if(has_extension(source_path, ".comp"))
            stage = ShaderStage::Compute;

        ShaderLanguage language;
        CompileOptions options;
#ifdef PLATFORM_MACOS
        options.is_apple_mobile = (bool)argv[3];

        language = ShaderLanguage::MSL;
#else
        language = ShaderLanguage::SPIRV;
#endif
        
        const auto compiled_source = shader_compiler.compile(ShaderLanguage::GLSL, stage, ShaderSource(buffer.str()), language, options);
        if(!compiled_source.has_value()) {
            prism::log::error(System::Core, "Error when compiling {}!", source_path);
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
    }

    // TODO: output disabled for now, will have to expose in a better arg system
    //prism::log::info(System::Core, "Successfully written shader from {} to {}.", source_path, destination_path);

    return 0;
}

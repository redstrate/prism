#include "asset.hpp"

#include <map>
#include <array>
#include <stb_image.h>

#include "log.hpp"
#include "engine.hpp"
#include "gfx.hpp"
#include "json_conversions.hpp"
#include "gfx_commandbuffer.hpp"
#include "assertions.hpp"
#include "renderer.hpp"
#include "input.hpp"
#include "physics.hpp"
#include "imgui_backend.hpp"

std::unique_ptr<Mesh> load_mesh(const prism::path path) {
    Expects(!path.empty());
    
    auto file = prism::open_file(path, true);
    if(!file.has_value()) {
        prism::log::error(System::Renderer, "Failed to load mesh from {}!", path);
        return nullptr;
    }

    int version = 0;
    file->read(&version);

    if(version == 5 || version == 6) {
    } else {
        prism::log::error(System::Renderer, "{} failed the mesh version check! reported version = {}", path, std::to_string(version));
        return nullptr;
    }

    auto mesh = std::make_unique<Mesh>();
    mesh->path = path.string();
    
    enum MeshType : int {
        Static,
        Skinned
    } mesh_type;
    
    file->read(&mesh_type);

    // TODO: use unsigned int here
    int numVertices = 0;
    file->read(&numVertices);
    
    Expects(numVertices > 0);
        
    const auto read_buffer = [&f = file.value(), numVertices](unsigned int size) -> GFXBuffer* {
        auto buffer = engine->get_gfx()->create_buffer(nullptr, size * static_cast<unsigned int>(numVertices), false, GFXBufferUsage::Vertex);
        auto buffer_ptr = reinterpret_cast<unsigned  char*>(engine->get_gfx()->get_buffer_contents(buffer));
        
        f.read(buffer_ptr, size * static_cast<unsigned int>(numVertices));
        
        engine->get_gfx()->release_buffer_contents(buffer, buffer_ptr);
        
        return buffer;
    };
    
    // read positions
    mesh->position_buffer = read_buffer(sizeof(prism::float3));
    mesh->normal_buffer = read_buffer(sizeof(prism::float3));
    mesh->texture_coord_buffer = read_buffer(sizeof(prism::float2));
    mesh->tangent_buffer = read_buffer(sizeof(prism::float3));
    mesh->bitangent_buffer = read_buffer(sizeof(prism::float3));

    if(mesh_type == MeshType::Skinned)
        mesh->bone_buffer = read_buffer(sizeof(BoneVertexData));
    
    int numIndices = 0;
    file->read(&numIndices);
    
    Expects(numIndices > 0);
    
    mesh->index_buffer = engine->get_gfx()->create_buffer(nullptr, sizeof(uint32_t) * numIndices, false, GFXBufferUsage::Index);
    auto index_ptr = reinterpret_cast<uint32_t*>(engine->get_gfx()->get_buffer_contents(mesh->index_buffer));

    file->read(index_ptr, sizeof(uint32_t) * numIndices);
    
    engine->get_gfx()->release_buffer_contents(mesh->index_buffer, index_ptr);

    int bone_len = 0;
    file->read(&bone_len);
    
    mesh->bones.reserve(bone_len);
    
    if(bone_len > 0) {
        file->read(&mesh->global_inverse_transformation);
            
        std::map<std::string, uint32_t> boneMapping;
        std::map<int, std::string> parentQueue;

        for (int v = 0; v < bone_len; v++) {
            std::string bone, parent;
            
            file->read_string(bone);
            file->read_string(parent);

            prism::float3 pos;
            file->read(&pos);
            
            Quaternion rot;
            file->read(&rot);

            prism::float3 scl;
            file->read(&scl);
                        
            if(!boneMapping.count(bone)) {
                Bone b;
                b.index = mesh->bones.size();
                b.name = bone;
                b.name = bone;
                b.position = pos;
                b.rotation = rot;
                b.scale = scl;
                
                if(parent != "none" && !parent.empty())
                    parentQueue[b.index] = parent;
                
                mesh->bones.push_back(b);
                
                boneMapping[bone] = b.index;
            }
        }
        
        for(auto& [index, parentName] : parentQueue) {
            for(auto& bone : mesh->bones) {
                if(bone.name == parentName)
                    mesh->bones[index].parent = &bone;
            }
        }
        
        for(auto& bone : mesh->bones) {
            if(bone.parent == nullptr)
                mesh->root_bone = &bone;
        }
    }
    
    int numMeshes = 0;
    file->read(&numMeshes);
    
    Expects(numMeshes > 0);
    
    mesh->parts.resize(numMeshes);

    uint32_t vertexOffset = 0, indexOffset = 0;
    for(int i = 0; i < numMeshes; i++) {
        auto& p = mesh->parts[i];
        p.vertex_offset = vertexOffset;
        p.index_offset = indexOffset;

        file->read_string(p.name);
        
        if(version == 6) {
            file->read(&p.bounding_box);
        }
        
        int numVerts = 0;
        file->read(&numVerts);
        
        file->read(&p.index_count);
        
        int numBones = 0;
        file->read(&numBones);
        
        p.bone_batrix_buffer = engine->get_gfx()->create_buffer(nullptr, sizeof(Matrix4x4) * 128, true, GFXBufferUsage::Storage);
        
        if(numBones > 0) {
            p.offset_matrices.resize(numBones);
            
            file->read(p.offset_matrices.data(), sizeof(Matrix4x4) * numBones);
        }

        file->read(&p.material_override);

        vertexOffset += numVerts;
        indexOffset += p.index_count;
    }

    mesh->num_indices = static_cast<uint32_t>(numIndices);

    return mesh;
}

std::unique_ptr<Texture> load_texture(const prism::path path) {
    Expects(!path.empty());
    
    auto file = prism::open_file(path, true);
    if(!file.has_value()) {
        prism::log::error(System::Renderer, "Failed to load texture from {}!", path);
        return nullptr;
    }
    
    // TODO: expose somehow??
    const bool should_generate_mipmaps = true;

    file->read_all();

    int width, height, channels;
    unsigned char* data = stbi_load_from_memory(file->cast_data<unsigned char>(), file->size(), &width, &height, &channels, 4);
    if(!data) {
        prism::log::error(System::Renderer, "Failed to load texture from {}!", path);
        return nullptr;
    }
    
    Expects(width > 0);
    Expects(height > 0);
    
    auto texture = std::make_unique<Texture>();
    texture->path = path.string();
    texture->width = width;
    texture->height = height;
    
    GFXTextureCreateInfo createInfo = {};
    createInfo.label = path.string();
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = GFXPixelFormat::R8G8B8A8_UNORM;
    createInfo.usage = GFXTextureUsage::Sampled;
    
    if(should_generate_mipmaps)
        createInfo.mip_count = std::floor(std::log2(std::max(width, height))) + 1;
    
    texture->handle = engine->get_gfx()->create_texture(createInfo);

    engine->get_gfx()->copy_texture(texture->handle, data, width * height * 4);
    
    if(createInfo.mip_count > 1) {
        GFXCommandBuffer* cmd_buf = engine->get_gfx()->acquire_command_buffer();
        
        
        cmd_buf->generate_mipmaps(texture->handle, createInfo.mip_count);
        
        engine->get_gfx()->submit(cmd_buf);
    }

    stbi_image_free(data);

    return texture;
}

std::unique_ptr<Material> load_material(const prism::path path) {
    Expects(!path.empty());
    
    auto file = prism::open_file(path);
    if(!file.has_value()) {
        prism::log::error(System::Core, "Failed to load material from {}!", path);
        return {};
    }
    
    nlohmann::json j;
    file->read_as_stream() >> j;

    auto mat = std::make_unique<Material>();
    mat->path = path.string();
    
    if(!j.count("version") || j["version"] != 2) {
        prism::log::error(System::Core, "Material {} failed the version check!", path);
        return mat;
    }
    
    for(auto node : j["nodes"]) {
        std::unique_ptr<MaterialNode> n;
        
        auto name = node["name"];
        if(name == "Material Output") {
            n = std::make_unique<MaterialOutput>();
        } else if(name == "Texture") {
            n = std::make_unique<TextureNode>();
        } else if(name == "Float Constant") {
            n = std::make_unique<FloatConstant>();
        }  else if(name == "Vector3 Constant") {
            n = std::make_unique<Vector3Constant>();
        }
        
        n->id = node["id"];
        n->x = node["x"];
        n->y = node["y"];
        
        for(auto& property : node["properties"]) {
            for(auto& p : n->properties) {
                if(property["name"] == p.name) {
                    p.value = property["value"];
                    p.float_value = property["float_value"];
                    
                    if(!property["asset_value"].get<std::string>().empty()) {
                        p.value_tex = assetm->get<Texture>(prism::app_domain / property["asset_value"].get<std::string>());
                    }
                }
            }
        }
        
        mat->nodes.emplace_back(std::move(n));
    }

    for(auto node : j["nodes"]) {
        MaterialNode* n = nullptr;
        for(auto& nn : mat->nodes) {
            if(nn->id == node["id"])
                n = nn.get();
        }
        
        if(n == nullptr)
            continue;
        
        for(auto connection : node["connections"]) {
            for(auto [i, output] : utility::enumerate(n->outputs)) {
                if(connection["name"] == output.name) {
                    for(auto& nn : mat->nodes) {
                        if(nn->id == connection["connected_node"]) {                            
                            output.connected_node = nn.get();
                            
                            auto connector = nn->find_connector(connection["connected_connector"]);
                            output.connected_connector = connector;
                            
                            connector->connected_index = i;
                            connector->connected_node = n;
                            connector->connected_connector = &output;
                        }
                    }
                    
                    output.connected_index = connection["connected_index"];
                }
            }
        }
    }
    
    return mat;
}

void save_material(Material* material, const prism::path path) {
    Expects(material != nullptr);
    Expects(!path.empty());
    
    nlohmann::json j;

    j["version"] = 2;

    for(auto& node : material->nodes) {
        nlohmann::json n;
        n["name"] = node->get_name();
        n["id"] = node->id;
        n["x"] = node->x;
        n["y"] = node->y;

        for(auto property : node->properties) {
            nlohmann::json p;
            p["name"] = property.name;
            p["value"] = property.value;
            p["asset_value"] = property.value_tex ? property.value_tex->path : "";
            p["float_value"] = property.float_value;
            
            n["properties"].push_back(p);
        }
        
        for(auto output : node->outputs) {
            if(output.connected_connector != nullptr) {
                nlohmann::json c;
                c["name"] = output.name;
                c["connected_connector"] = output.connected_connector->name;
                c["connected_node"] = output.connected_node->id;
                c["connected_index"] = output.connected_index;
                
                n["connections"].push_back(c);
            }
        }
        
        j["nodes"].push_back(n);
    }

    std::ofstream out(path);
    out << j;
}

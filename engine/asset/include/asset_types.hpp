#pragma once

#include <map>

#include "assetptr.hpp"
#include "math.hpp"
#include "material_nodes.hpp"
#include "aabb.hpp"
#include "utility.hpp"

class GFXBuffer;
class GFXTexture;

class Texture : public Asset {
public:    
    GFXTexture* handle = nullptr;
    int width = 0, height = 0;
};

class GFXPipeline;

class Material : public Asset {
public:
    std::vector<std::unique_ptr<MaterialNode>> nodes;
    
    void delete_node(MaterialNode* node) {
        // disconnect all inputs from their node outputs
        for(auto& input : node->inputs) {
            if(input.is_connected())
                input.disconnect();
        }
        
        // disconnect all our outputs from other node's inputs
        for(auto& output : node->outputs) {
            if(output.is_connected())
                output.connected_connector->disconnect();
        }
        
        utility::erase_if(nodes, [node](std::unique_ptr<MaterialNode>& other_node) {
            return node == other_node.get();
        });
    }
    
    GFXPipeline* static_pipeline = nullptr;
    GFXPipeline* skinned_pipeline = nullptr;
    GFXPipeline* capture_pipeline = nullptr;

    std::map<int, AssetPtr<Texture>> bound_textures;
};

constexpr int max_weights_per_vertex = 4;

struct BoneVertexData {
    std::array<int, max_weights_per_vertex> ids;
    std::array<float, max_weights_per_vertex> weights;
};

struct Bone {
    int index = 0;

    std::string name;
    Matrix4x4 local_transform, final_transform;

    Bone* parent = nullptr;

    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
};

class Mesh : public Asset {
public:
    // meshes are rendered in parts if we cannot batch it in one call, i.e. a mesh
    // with multiple materials with different textures, etc
    struct Part {
        std::string name;
        AABB aabb;
        
        GFXBuffer* bone_batrix_buffer = nullptr;
        std::vector<Matrix4x4> offset_matrices;
        
        uint32_t index_offset = 0, vertex_offset = 0, index_count = 0;
        int32_t material_override = -1;
    };

    std::vector<Part> parts;
    std::vector<Bone> bones;
    Bone* root_bone = nullptr;

    // atributes
    GFXBuffer* position_buffer = nullptr;
    GFXBuffer* normal_buffer = nullptr;
    GFXBuffer* texture_coord_buffer = nullptr;
    GFXBuffer* tangent_buffer = nullptr;
    GFXBuffer* bitangent_buffer = nullptr;
    GFXBuffer* bone_buffer = nullptr;
    
    GFXBuffer* index_buffer = nullptr;
    
    Matrix4x4 global_inverse_transformation;

    uint32_t num_indices = 0;
};

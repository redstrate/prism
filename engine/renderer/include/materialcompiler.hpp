#pragma once

#include <tuple>

#include "gfx.hpp"

class Material;

constexpr int position_buffer_index = 2;
constexpr int normal_buffer_index = 3;
constexpr int texcoord_buffer_index = 4;
constexpr int tangent_buffer_index = 5;
constexpr int bitangent_buffer_index = 6;
constexpr int bone_buffer_index = 7;

class MaterialCompiler {
public:
    GFXPipeline* create_static_pipeline(GFXGraphicsPipelineCreateInfo createInfo, bool positions_only = false, bool cubemap = false);
    GFXPipeline* create_skinned_pipeline(GFXGraphicsPipelineCreateInfo createInfo, bool positions_only = false);

    // generates static and skinned versions of the pipeline provided
    std::tuple<GFXPipeline*, GFXPipeline*> create_pipeline_permutations(GFXGraphicsPipelineCreateInfo& createInfo, bool positions_only = false);
    
    std::variant<std::string, std::vector<uint32_t>> compile_material_fragment(Material& material, bool use_ibl = true);
};

static MaterialCompiler material_compiler;

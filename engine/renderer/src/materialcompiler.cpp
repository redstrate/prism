#include "materialcompiler.hpp"

#include <cstring>

#include "file.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "string_utils.hpp"
#include "shadercompiler.hpp"
#include "material_nodes.hpp"
#include "renderer.hpp"

ShaderSource get_shader(std::string filename, bool skinned, bool cubemap) {
    auto shader_file = file::open(file::internal_domain / filename);
    if(!shader_file.has_value()) {
        console::error(System::Renderer, "Failed to open shader file {}!", filename);
        return {};
    }
    
    ShaderStage stage;
    if(filename.find("vert") != std::string::npos) {
        stage = ShaderStage::Vertex;
    } else {
        stage = ShaderStage::Fragment;
    }
    
    CompileOptions options;
    if(skinned)
        options.add_definition("BONE");
    
    if(cubemap)
        options.add_definition("CUBEMAP");
    
    return *shader_compiler.compile(ShaderLanguage::GLSL, stage, shader_file->read_as_string(), engine->get_gfx()->accepted_shader_language(), options);
}

GFXPipeline* MaterialCompiler::create_static_pipeline(GFXGraphicsPipelineCreateInfo createInfo, bool positions_only, bool cubemap) {
    // take vertex src
    std::string vertex_path = createInfo.shaders.vertex_src.as_path().string();
    vertex_path += ".glsl";

    if (positions_only)
        createInfo.label += "shadow ver";

    if (cubemap)
        createInfo.label += "cubemap ver";
    
    createInfo.shaders.vertex_src = get_shader(vertex_path, false, cubemap);
    
    if(positions_only) {
        createInfo.vertex_input.inputs = {
            {position_buffer_index, sizeof(Vector3)}
        };
        
        createInfo.vertex_input.attributes = {
            {position_buffer_index, 0, 0, GFXVertexFormat::FLOAT3}
        };
    } else {
        createInfo.vertex_input.inputs = {
            {position_buffer_index, sizeof(Vector3)},
            {normal_buffer_index, sizeof(Vector3)},
            {texcoord_buffer_index, sizeof(Vector2)},
            {tangent_buffer_index, sizeof(Vector3)},
            {bitangent_buffer_index, sizeof(Vector3)}
        };
        
        createInfo.vertex_input.attributes = {
            {position_buffer_index, 0, 0, GFXVertexFormat::FLOAT3},
            {normal_buffer_index, 1, 0, GFXVertexFormat::FLOAT3},
            {texcoord_buffer_index, 2, 0, GFXVertexFormat::FLOAT2},
            {tangent_buffer_index, 3, 0, GFXVertexFormat::FLOAT3},
            {bitangent_buffer_index, 4, 0, GFXVertexFormat::FLOAT3}
        };
    }
    
    return engine->get_gfx()->create_graphics_pipeline(createInfo);
}

GFXPipeline* MaterialCompiler::create_skinned_pipeline(GFXGraphicsPipelineCreateInfo createInfo, bool positions_only) {
    createInfo.label += " (Skinned)";
    
    // take vertex src
    std::string vertex_path = createInfo.shaders.vertex_src.as_path().string();
    vertex_path += ".glsl";
    
    createInfo.shaders.vertex_src = get_shader(vertex_path, true, false);
     
    createInfo.shader_input.bindings.push_back({ 14, GFXBindingType::StorageBuffer });
    
    if(positions_only) {
        createInfo.vertex_input.inputs = {
            {position_buffer_index, sizeof(Vector3)},
            {bone_buffer_index, sizeof(BoneVertexData)}
        };
        
        createInfo.vertex_input.attributes = {
            {position_buffer_index, 0, 0, GFXVertexFormat::FLOAT3},
            {bone_buffer_index, 4, offsetof(BoneVertexData, ids), GFXVertexFormat::INT4},
            {bone_buffer_index, 5, offsetof(BoneVertexData, weights), GFXVertexFormat::FLOAT4}
        };
    } else {
        createInfo.vertex_input.inputs = {
            {position_buffer_index, sizeof(Vector3)},
            {normal_buffer_index, sizeof(Vector3)},
            {texcoord_buffer_index, sizeof(Vector2)},
            {tangent_buffer_index, sizeof(Vector3)},
            {bitangent_buffer_index, sizeof(Vector3)},
            {bone_buffer_index, sizeof(BoneVertexData)}
        };
        
        createInfo.vertex_input.attributes = {
            {position_buffer_index, 0, 0, GFXVertexFormat::FLOAT3},
            {normal_buffer_index, 1, 0, GFXVertexFormat::FLOAT3},
            {texcoord_buffer_index, 2, 0, GFXVertexFormat::FLOAT2},
            {tangent_buffer_index, 3, 0, GFXVertexFormat::FLOAT3},
            {bitangent_buffer_index, 4, 0, GFXVertexFormat::FLOAT3},
            {bone_buffer_index, 5, offsetof(BoneVertexData, ids), GFXVertexFormat::INT4},
            {bone_buffer_index, 6, offsetof(BoneVertexData, weights), GFXVertexFormat::FLOAT4},
        };
    }
    
    return engine->get_gfx()->create_graphics_pipeline(createInfo);
}

std::tuple<GFXPipeline*, GFXPipeline*> MaterialCompiler::create_pipeline_permutations(GFXGraphicsPipelineCreateInfo& createInfo, bool positions_only) {
    auto st = create_static_pipeline(createInfo, positions_only);
    auto ss = create_skinned_pipeline(createInfo, positions_only);
    
    return {st, ss};
}

std::vector<MaterialNode*> walked_nodes;

void walk_node(std::string& src, MaterialNode* node) {
    if(utility::contains(walked_nodes, node))
        return;
    
    walked_nodes.push_back(node);
    
    for(auto& input : node->inputs) {
        if(input.connected_node != nullptr)
            walk_node(src, input.connected_node);
    }
    
    std::string intermediate = node->get_glsl();
    for(auto& property : node->properties) {
        intermediate = replace_substring(intermediate, property.name, node->get_property_value(property));
    }
    
    for(auto& input : node->inputs) {
        intermediate = replace_substring(intermediate, input.name, node->get_connector_value(input));
    }
    
    for(auto& output : node->outputs) {
        intermediate = replace_substring(intermediate, output.name, node->get_connector_variable_name(output));
    }
    
    src += intermediate;
}

constexpr std::string_view struct_info =
"layout (constant_id = 0) const int max_materials = 25;\n \
layout (constant_id = 1) const int max_lights = 25;\n \
layout (constant_id = 2) const int max_spot_lights = 4;\n \
layout (constant_id = 3) const int max_probes = 4;\n \
struct Material {\n \
    vec4 color, info;\n \
};\n \
struct Light {\n \
    vec4 positionType;\n \
    vec4 directionPower;\n \
    vec4 colorSize;\n \
    vec4 shadowsEnable;\n \
};\n \
struct Probe {\n \
    vec4 position, size;\n \
};\n \
layout(std430, binding = 1) buffer readonly SceneInformation {\n \
    vec4 options;\n \
    vec4 camPos;\n \
    mat4 vp, lightSpace;\n \
    mat4 spotLightSpaces[max_spot_lights];\n \
    Material materials[max_materials];\n \
    Light lights[max_lights];\n \
    Probe probes[max_probes];\n \
    int numLights;\n \
} scene;\n \
layout (binding = 2) uniform sampler2D sun_shadow;\n \
layout (binding = 6) uniform sampler2DArray spot_shadow;\n \
layout(push_constant, binding = 0) uniform PushConstant {\n \
    mat4 model;\n \
};\n";

ShaderSource MaterialCompiler::compile_material_fragment(Material& material, bool use_ibl) {
    walked_nodes.clear();
    
    if(!render_options.enable_ibl)
        use_ibl = false;
    
    std::string src;
    
    switch(render_options.shadow_filter) {
        case ShadowFilter::None:
            src += "#define SHADOW_FILTER_NONE\n";
            break;
        case ShadowFilter::PCF:
            src += "#define SHADOW_FILTER_PCF\n";
            break;
        case ShadowFilter::PCSS:
            src += "#define SHADOW_FILTER_PCSS\n";
            break;
    }
    
    src += "layout (location = 0) in vec3 in_frag_pos;\n";
    src += "layout(location = 1) in vec3 in_normal;\n";
    src += "layout(location = 2) in vec2 in_uv;\n";

    src += "layout(location = 0) out vec4 frag_output;\n";
    
    if(render_options.enable_point_shadows) {
        src += "#define POINT_SHADOWS_SUPPORTED\n";
        src += "layout (binding = 3) uniform samplerCubeArray point_shadow;\n";
    }

    src += struct_info;
    
    if(use_ibl) {
        src += "layout (binding = 7) uniform samplerCubeArray irrandianceSampler;\n \
        layout (binding = 8) uniform samplerCubeArray prefilterSampler;\n \
        layout (binding = 9) uniform sampler2D brdfSampler;\n";
    }
    
    src += "layout(location = 4) in vec4 fragPosLightSpace;\n";
    src += "layout(location = 5) in mat3 in_tbn;\n";
    src += "layout(location = 14) in vec4 fragPostSpotLightSpace[max_spot_lights];\n";
    
    src += "#include \"common.glsl\"\n";
    src += "#include \"rendering.glsl\"\n";
    
    material.bound_textures.clear();
    
    // insert samplers as needed
    int sampler_index = 10;
    for(auto& node : material.nodes) {
        for(auto& property : node->properties) {
            if(property.type == DataType::AssetTexture) {
                material.bound_textures[sampler_index] = property.value_tex;
                
                src += "layout(binding = " + std::to_string(sampler_index++) + ") uniform sampler2D " + node->get_property_variable_name(property) + ";\n";
            }
        }
    }
    
    if(use_ibl) {
        src += "vec3 get_reflect(int i, vec3 final_normal) {\n \
        const vec3 direction = normalize(in_frag_pos - scene.camPos.xyz);\n \
        const vec3 reflection = reflect(direction, normalize(final_normal));\n \
        vec3 box_max = scene.probes[i].position.xyz + (scene.probes[i].size.xyz / 2.0f);\n \
        vec3 box_min = scene.probes[i].position.xyz + -(scene.probes[i].size.xyz / 2.0f);\n \
        vec3 unitary = vec3(1.0);\n \
        vec3 first_plane_intersect = (box_max - in_frag_pos) / reflection;\n \
        vec3 second_plane_intersect = (box_min - in_frag_pos) / reflection;\n \
        vec3 furthest_plane = max(first_plane_intersect, second_plane_intersect);\n \
        float distance = min(furthest_plane.x, min(furthest_plane.y, furthest_plane.z));\n \
        vec3 intersect_position_world = in_frag_pos + reflection * distance; \n \
        return intersect_position_world - scene.probes[i].position.xyz;\n}\n";
    }
    
    if(render_options.enable_normal_shadowing) {
        src += "float calculate_normal_lighting(in sampler2D normal_map, const vec3 normal, const vec3 light_dir) {\n \
            float height_scale = 0.8;\n \
            float sample_count = 100.0;\n \
            float inv_sample_count = 1.0 / sample_count;\n \
            float hardness = 50 * 0.5;\n \
            float lighting = clamp(dot(light_dir, normal), 0.0, 1.0);\n \
            float slope = -lighting;\n \
            vec2 dir = light_dir.xy * vec2(1.0, -1.0) * height_scale;\n \
            float max_slope = 0.0;\n \
            float step = inv_sample_count;\n \
            float pos = step;\n \
            pos = (-lighting >= 0.0) ? 1.001 : pos;\n \
            vec2 noise = fract(in_frag_pos.xy * 0.5);\n \
            noise.x = noise.x + noise.y * 0.5;\n \
            pos = step - step * noise.x;\n \
            float shadow = 0.0;\n \
            while(pos <= 1.0) {\n \
                vec3 tmp_normal = texture(normal_map, in_uv + dir * pos).rgb;\n \
                tmp_normal = in_tbn * (tmp_normal * 2.0 - 1.0);\n \
                float tmp_lighting = dot(light_dir, tmp_normal);\n \
                float shadowed = -tmp_lighting;\n \
                slope += shadowed;\n \
                if(slope > max_slope) {\n \
                    shadow += hardness * (1.0 - pos);\n \
                }\n \
                max_slope = max(max_slope, slope);\n \
                pos += step;\n \
            }\n \
            return clamp(1.0 - shadow * inv_sample_count, 0.0, 1.0);\n \
            }\n";
    }
    
    if(use_ibl) {
        src += "vec3 ibl(const int probe, const ComputedSurfaceInfo surface_info, const float intensity) {\n \
            const vec3 F = fresnel_schlick_roughness(surface_info.NdotV, surface_info.F0, surface_info.roughness);\n \
            const vec3 R = get_reflect(probe, surface_info.N);\n \
            const vec2 brdf = texture(brdfSampler, vec2(surface_info.NdotV, surface_info.roughness)).rg; \n \
            const vec3 sampledIrradiance = texture(irrandianceSampler, vec4(surface_info.N, probe)).xyz;\n \
            const vec3 prefilteredColor = textureLod(prefilterSampler, vec4(R, probe), surface_info.roughness * 4).xyz;\n \
            const vec3 diffuse = sampledIrradiance * surface_info.diffuse_color;\n \
            const vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);\n \
            return (diffuse + specular) * intensity;\n \
        }\n";
    }
    
    src += "void main() {\n";
    
    bool has_output = false;
    bool has_normal_mapping = false;
    std::string normal_map_property_name;
    for(auto& node : material.nodes) {
        if(!strcmp(node->get_name(), "Material Output")) {
            for(auto& input : node->inputs) {
                if(input.is_normal_map && input.connected_node != nullptr) {
                    has_normal_mapping = true;
                    normal_map_property_name = input.connected_node->get_property_variable_name(input.connected_node->properties[0]); // quick and dirty workaround to get the normal map texture name
                }
            }
            
            walk_node(src, node.get());
            has_output = true;
        }
    }
    
    if(!has_output) {
        src += "vec3 final_diffuse_color = vec3(1);\n";
        src += "float final_roughness = 0.5;\n";
        src += "float final_metallic = 0.0;\n";
        src += "vec3 final_normal = in_normal;\n";
    }
    
    src +=
    "ComputedSurfaceInfo surface_info = compute_surface(final_diffuse_color.rgb, final_normal, final_metallic, final_roughness);\n \
    vec3 Lo = vec3(0);\n \
    for(int i = 0; i < scene.numLights; i++) {\n \
        const int type = int(scene.lights[i].positionType.w);\n \
        ComputedLightInformation light_info;\n \
        switch(type) {\n \
            case 0:\n \
                light_info = calculate_point(scene.lights[i]);\n \
                break;\n \
            case 1:\n \
                light_info = calculate_spot(scene.lights[i]);\n \
                break;\n \
            case 2:\n \
                light_info = calculate_sun(scene.lights[i]);\n \
                break;\n \
        }\n \
    SurfaceBRDF surface_brdf = brdf(light_info.direction, surface_info);\n";
    
    if(render_options.enable_normal_mapping && has_normal_mapping && render_options.enable_normal_shadowing) {
        src += std::string("light_info.radiance *= calculate_normal_lighting(") + normal_map_property_name + ", final_normal, light_info.direction);\n";
    }
        
    src += "Lo += ((surface_brdf.specular + surface_brdf.diffuse) * light_info.radiance * surface_brdf.NdotL) * scene.lights[i].colorSize.rgb;\n \
    }\n";
        
    if(use_ibl) {
        src +=
        "vec3 ambient = vec3(0.0); \
        float sum = 0.0;\n \
        for(int i = 0; i < max_probes; i++) { \
            if(scene.probes[i].position.w == 1) {\n \
                const vec3 position = scene.probes[i].position.xyz; \
                const vec3 probe_min = position - (scene.probes[i].size.xyz / 2.0); \
                const vec3 probe_max = position + (scene.probes[i].size.xyz / 2.0); \
                if(all(greaterThan(in_frag_pos, probe_min)) && all(lessThan(in_frag_pos, probe_max))) { \
                    float intensity = 1.0 - length(abs(in_frag_pos - position) / (scene.probes[i].size.xyz / 2.0));\n \
                    intensity = clamp(intensity, 0.0, 1.0) * scene.probes[i].size.w;\n \
                    ambient += ibl(i, surface_info, intensity);\n \
                    sum += intensity; \n \
                } \
            } else if(scene.probes[i].position.w == 2) {\n \
                ambient += ibl(i, surface_info, scene.probes[i].size.w);\n \
                sum += scene.probes[i].size.w; \n \
            }\n \
        }\n \
        ambient /= sum;\n";
        
        src += "frag_output = vec4(ambient + Lo, 1.0);\n";
    } else {
        src += "frag_output = vec4(Lo, 1.0);\n";
    }
    
    src += "}\n";
            
    return *shader_compiler.compile(ShaderLanguage::GLSL, ShaderStage::Fragment, src, engine->get_gfx()->accepted_shader_language());
}

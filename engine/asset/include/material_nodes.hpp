#pragma once

#include "assetptr.hpp"
#include "render_options.hpp"

class MaterialNode;

enum class DataType {
    Vector3,
    Float,
    AssetTexture
};

struct MaterialConnector {
    MaterialConnector(std::string n, DataType t) : name(n), type(t) {}
    MaterialConnector(std::string n, DataType t, bool isn) : name(n), type(t), is_normal_map(isn) {}
    
    bool is_connected() const {
        return connected_connector != nullptr && connected_node != nullptr && connected_index != -1;
    }
    
    void disconnect() {
        connected_connector = nullptr;
        connected_node = nullptr;
        connected_index = -1;
    }
    
    std::string name;
    DataType type;

    bool is_normal_map = false;
    
    MaterialConnector* connected_connector = nullptr;
    MaterialNode* connected_node = nullptr;
    int connected_index = -1;
};

inline bool operator==(const MaterialConnector& a, const MaterialConnector& b) {
    return a.name == b.name;
}

class Texture;

struct MaterialProperty {
    std::string name;
    DataType type;

    MaterialProperty(std::string n, DataType t) : name(n), type(t) {}
    
    Vector3 value;
    AssetPtr<Texture> value_tex;
    float float_value;
};

static int last_id = 0;

class MaterialNode {
public:
    virtual ~MaterialNode() {}
    
    int id = last_id++;
    
    std::vector<MaterialConnector> inputs, outputs;
    std::vector<MaterialProperty> properties;
    
    virtual const char* get_prefix() = 0;
    virtual const char* get_name() = 0;
    
    virtual std::string get_glsl() = 0;
    
    std::string get_connector_variable_name(MaterialConnector& connector) {
        return std::string(get_prefix()) + std::to_string(id) + "_" + connector.name;
    }
    
    std::string get_property_variable_name(MaterialProperty& property) {
        return std::string(get_prefix()) + std::to_string(id) + "_" + property.name;
    }
    
    std::string get_connector_value(MaterialConnector& connector) {
        if(connector.connected_node != nullptr) {
            if(connector.type != connector.connected_connector->type) {
                return connector.connected_node->get_connector_variable_name(*connector.connected_connector) + ".r";
            } else {
                if(connector.is_normal_map) {
                    if(render_options.enable_normal_mapping) {
                        return "in_tbn * (2.0 * " + connector.connected_node->get_connector_variable_name(*connector.connected_connector) + " - 1.0)";
                    } else {
                        return "in_normal";
                    }
                } else {
                    return connector.connected_node->get_connector_variable_name(*connector.connected_connector);
                }
            }
        } else {
            switch(connector.type) {
                case DataType::Float:
                    return "0.0";
                default:
                    return "vec3(1, 1, 1)";
            }
        }
    }
    
    std::string get_property_value(MaterialProperty& property) {
        switch(property.type) {
            case DataType::Vector3:
            {
                return "vec3(" + std::to_string(property.value.x) + ", " + std::to_string(property.value.y) + ", " + std::to_string(property.value.z) + ")";
            }
                break;
            case DataType::Float:
            {
                return std::to_string(property.float_value);
            }
                break;
            case DataType::AssetTexture:
            {
                return "texture(" + get_property_variable_name(property) + ", in_uv).rgb";
            }
                break;
        }
    }
    
    MaterialConnector* find_connector(std::string name) {
        for(auto& input : inputs) {
            if(input.name == name)
                return &input;
        }
        
        for(auto& output : outputs) {
            if(output.name == name)
                return &output;
        }
        
        return nullptr;
    }
    
    float x = 0.0f, y = 0.0f;
    float width = 150.0f, height = 75.0f;
};

class MaterialOutput : public MaterialNode {
public:
    MaterialOutput() {
        inputs = {
            MaterialConnector("Color", DataType::Vector3),
            MaterialConnector("Roughness", DataType::Float),
            MaterialConnector("Metallic", DataType::Float),
            MaterialConnector("Normals", DataType::Vector3, true),
        };
    }
    
    const char* get_prefix() override {
        return "material_output_";
    }
    
    const char* get_name() override {
        return "Material Output";
    }
    
    std::string get_glsl() override {
        std::string glsl = "vec3 final_diffuse_color = from_srgb_to_linear(Color);\n \
        float final_roughness = Roughness;\n \
        float final_metallic = Metallic;\n";
        
        if(find_connector("Normals")->connected_node != nullptr) {
            glsl += "vec3 final_normal = Normals;\n";
        } else {
            glsl += "vec3 final_normal = in_normal;\n";
        }
        
        return glsl;
    }
};

class Vector3Constant : public MaterialNode {
public:
    Vector3Constant() {
        outputs = {MaterialConnector("Value", DataType::Vector3)};
        properties = {MaterialProperty("Color", DataType::Vector3)};
    }
    
    const char* get_prefix() override {
        return "vec3const_";
    }
    
    const char* get_name() override {
        return "Vector3 Constant";
    }
    
    std::string get_glsl() override {
        return "vec3 Value = Color;\n";
    }
};

class FloatConstant : public MaterialNode {
public:
    FloatConstant() {
        outputs = {MaterialConnector("Value", DataType::Float)};
        properties = {MaterialProperty("Data", DataType::Float)};
    }
    
    const char* get_prefix() override {
        return "floatconst_";
    }
    
    const char* get_name() override {
        return "Float Constant";
    }
    
    std::string get_glsl() override {
        return "float Value = Data;\n";
    }
};

class TextureNode : public MaterialNode {
public:
    TextureNode() {
        outputs = {MaterialConnector("Value", DataType::Vector3)};
        properties = {MaterialProperty("Texture", DataType::AssetTexture)};
    }
    
    const char* get_prefix() override {
        return "texture_";
    }
    
    const char* get_name() override {
        return "Texture";
    }
    
    std::string get_glsl() override {
        return "vec3 Value = Texture;\n";
    }
};

class Geometry : public MaterialNode {
public:
    Geometry() {
        outputs = {MaterialConnector("Normal", DataType::Vector3)};
    }
    
    const char* get_prefix() override {
        return "geometry_";
    }
    
    const char* get_name() override {
        return "Geometry";
    }
    
    std::string get_glsl() override {
        return "vec3 Normal = normalize(in_normal);\n";
    }
};

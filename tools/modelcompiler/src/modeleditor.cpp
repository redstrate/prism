#include "modeleditor.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include <magic_enum.hpp>
#include <cstdio>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_map>
#include <functional>

#include "engine.hpp"
#include "imguipass.hpp"
#include "file.hpp"
#include "json_conversions.hpp"
#include "platform.hpp"
#include "utility.hpp"

void app_main(Engine* engine) {
    ModelEditor* editor = (ModelEditor*)engine->get_app();

    if(utility::contains(engine->command_line_arguments, "--no_ui"))
        editor->flags.hide_ui = true;

    if(!editor->flags.hide_ui) {
        platform::open_window("Model Compiler",
                              {editor->getDefaultX(), editor->getDefaultY(), 300, 300},
                              WindowFlags::None);
    } else {
        for(auto [i, argument] : utility::enumerate(engine->command_line_arguments)) {
            if(argument == "--model-path")
                editor->model_path = engine->command_line_arguments[i + 1];

            if(argument == "--data-path")
                editor->data_path = engine->command_line_arguments[i + 1];

            if(argument == "--compile-static")
                editor->flags.compile_static = true;
        }

        editor->compile_model();

        platform::force_quit();
    }
}

ModelEditor::ModelEditor() : CommonEditor("ModelEditor") {}

void traverseNode(const aiScene* scene, aiNode* node, const aiBone* bone, aiString& parent) {
    for(int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if(child->mName == bone->mName) {
            parent = node->mName;
        } else {
            traverseNode(scene, child, bone, parent);
        }
    }
}

void traverseNode(const aiScene* scene, aiNode* node, const aiBone* bone, aiVector3D& pos) {
    for(int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if(child->mName == bone->mName) {
            aiVector3D scale, position;
            aiQuaternion rotation;

            child->mTransformation.Decompose(scale, rotation, position);

            pos = position;
        } else {
            traverseNode(scene, child, bone, pos);
        }
    }
}

void traverseNodeScale(const aiScene* scene, aiNode* node, const aiBone* bone, aiVector3D& pos) {
    for(int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if(child->mName == bone->mName) {
            aiVector3D scale, position;
            aiQuaternion rotation;

            child->mTransformation.Decompose(scale, rotation, position);

            pos = scale;
        } else {
            traverseNodeScale(scene, child, bone, pos);
        }
    }
}

void traverseNode(const aiScene* scene, aiNode* node, const aiBone* bone, aiQuaternion& rot) {
    for(int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if(child->mName == bone->mName) {
            aiVector3D scale, position;
            aiQuaternion rotation;

            child->mTransformation.Decompose(scale, rotation, position);

            rot = rotation;
        } else {
            traverseNode(scene, child, bone, rot);
        }
    }
}

aiString findBoneParent(const aiScene* scene, const aiBone* bone) {
    aiString t("none");

    traverseNode(scene, scene->mRootNode, bone, t);

    return t;
}

aiVector3D getBonePosition(const aiScene* scene, const aiBone* bone) {
    aiVector3D pos;

    traverseNode(scene, scene->mRootNode, bone, pos);

    return pos;
}

aiQuaternion getBoneRotation(const aiScene* scene, const aiBone* bone) {
    aiQuaternion pos;

    traverseNode(scene, scene->mRootNode, bone, pos);

    return pos;
}

aiVector3D getBoneScale(const aiScene* scene, const aiBone* bone) {
    aiVector3D pos;

    traverseNodeScale(scene, scene->mRootNode, bone, pos);

    return pos;
}

void write_string(FILE* file, const aiString& str) {
    fwrite(&str.length, sizeof(unsigned int), 1, file);
    fwrite(str.C_Str(), sizeof(char) * str.length, 1, file);
}

void ModelEditor::compile_model() {
    struct BoneWeight {
        unsigned int vertex_index = 0, bone_index = 0;
        float weight = 0.0f;
    };
    
    constexpr int max_weights_per_vertex = 4;
    
    struct BoneVertexData {
        std::array<int, max_weights_per_vertex> ids;
        std::array<float, max_weights_per_vertex> weights;
        
        void add(int boneID, float boneWeight) {
            for (int i = 0; i < max_weights_per_vertex; i++) {
                if (weights[i] == 0.0f) {
                    ids[i] = boneID;
                    weights[i] = boneWeight;
                    return;
                }
            }
        }
    };
    
    Assimp::Importer importer;

    unsigned int importer_flags =
        aiProcess_Triangulate |
        aiProcess_ImproveCacheLocality |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_CalcTangentSpace;

    if(flags.compile_static) {
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.0);
        
        importer_flags |= aiProcess_GlobalScale;
        importer_flags |= aiProcess_PreTransformVertices;
        
        aiMatrix4x4 matrix;
        
        importer.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, matrix);
        importer.SetPropertyBool(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, true);
    }
    
    const aiScene* sc = importer.ReadFile(model_path.c_str(), importer_flags);
    
    auto name = model_path.substr(model_path.find_last_of("/") + 1, model_path.length());
    name = name.substr(0, name.find_last_of("."));

    FILE* file = fopen((data_path + "/models/" + name + ".model").c_str(), "wb");

    int version = 6;
    fwrite(&version, sizeof(int), 1, file);
    
    std::vector<std::string> meshToMaterial;
    meshToMaterial.resize(sc->mNumMeshes);

    std::map<std::string, int> matNameToIndex;
    int last_material = 0;
    
    std::vector<aiVector3D> positions;
    
    enum MeshType : int {
        Static,
        Skinned
    } mesh_type = MeshType::Static;
    
    std::vector<aiVector3D> normals;
    std::vector<aiVector2D> texture_coords;
    std::vector<aiVector3D> tangents, bitangents;
    
    std::vector<BoneVertexData> bone_vertex_data;
    
    for(unsigned int i = 0; i < sc->mNumMeshes; i++) {
        aiMesh* mesh = sc->mMeshes[i];
        
        if(mesh->HasBones())
            mesh_type = MeshType::Skinned;
    }
    
    std::vector<uint32_t> indices;
    
    std::vector<BoneWeight> bone_weights;

    aiMesh* armature_mesh = nullptr;

    int vertex_offset = 0;
    for(unsigned int i = 0; i < sc->mNumMeshes; i++) {
        aiMesh* mesh = sc->mMeshes[i];

        if(armature_mesh == nullptr && mesh->mNumBones != 0) {
            armature_mesh = mesh;
        }
        
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            positions.push_back(mesh->mVertices[v]);
            normals.push_back(mesh->mNormals[v]);
            
            if(mesh->HasTextureCoords(0)) {
                texture_coords.emplace_back(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
            } else {
                texture_coords.emplace_back(0, 0);
            }
            
            tangents.push_back(mesh->mTangents[v]);
            bitangents.push_back(mesh->mBitangents[v]);
        }
        
        for (unsigned int e = 0; e < mesh->mNumFaces; e++) {
            const aiFace& face = mesh->mFaces[e];
            
            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
        
        for(unsigned int b = 0; b < mesh->mNumBones; b++) {
            for(int y = 0; y < mesh->mBones[b]->mNumWeights; y++) {
                BoneWeight bw;
                bw.bone_index = b;
                bw.vertex_index = vertex_offset + mesh->mBones[b]->mWeights[y].mVertexId;
                bw.weight = mesh->mBones[b]->mWeights[y].mWeight;
                
                bone_weights.push_back(bw);
            }
        }
        
        std::string material = sc->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
        meshToMaterial[i] = material;
        
        if(!matNameToIndex.count(material))
            matNameToIndex[material] = last_material++;
        
        vertex_offset += mesh->mNumVertices;
    }
    
    if(mesh_type == MeshType::Skinned) {
        bone_vertex_data.resize(positions.size());
        
        for(auto bw : bone_weights)
            bone_vertex_data[bw.vertex_index].add(bw.bone_index, bw.weight);
    }
    
    fwrite(&mesh_type, sizeof(int), 1, file);
    
    int vert_len = positions.size();
    fwrite(&vert_len, sizeof(int), 1, file);
    
    const auto write_buffer = [numVertices = positions.size(), file](auto vec, unsigned int size) {
        for(unsigned int i = 0; i < numVertices; i++) {
            fwrite(&vec[i], size, 1, file);
        }
    };
    
    write_buffer(positions, sizeof(aiVector3D));
    write_buffer(normals, sizeof(aiVector3D));
    write_buffer(texture_coords, sizeof(aiVector2D));
    write_buffer(tangents, sizeof(aiVector3D));
    write_buffer(bitangents, sizeof(aiVector3D));

    if(mesh_type == MeshType::Skinned)
        write_buffer(bone_vertex_data, sizeof(BoneVertexData));
    
    int element_len = indices.size();
    fwrite(&element_len, sizeof(int), 1, file);
    
    fwrite(indices.data(), sizeof(uint32_t) * indices.size(), 1, file);
    
    int bone_len = 0;
    if(armature_mesh != nullptr) {
        bone_len = armature_mesh->mNumBones;
    }
    
    fwrite(&bone_len, sizeof(int), 1, file);
    
    if(bone_len > 0) {
        auto transform = sc->mRootNode->mTransformation;
        transform.Inverse();
        transform.Transpose();
        
        fwrite(&transform, sizeof(aiMatrix4x4), 1, file);
        
        for (unsigned int b = 0; b < armature_mesh->mNumBones; b++) {
            const aiBone* aBone = armature_mesh->mBones[b];
            
            write_string(file, aBone->mName);
            write_string(file, findBoneParent(sc, aBone));
            
            auto pos = getBonePosition(sc, aBone);
            
            fwrite(&pos, sizeof(float) * 3, 1, file);
            
            auto rot = getBoneRotation(sc, aBone);
            
            fwrite(&rot, sizeof(float) * 4, 1, file);
            
            auto scl = getBoneScale(sc, aBone);
            
            fwrite(&scl, sizeof(float) * 3, 1, file);
        }
    }
    
    fwrite(&sc->mNumMeshes, sizeof(int), 1, file);
    
    for (unsigned int i = 0; i < sc->mNumMeshes; i++) {
        const aiMesh* mesh = sc->mMeshes[i];

        write_string(file, mesh->mName);
        
        float min_x = 0.0f, min_y = 0.0f, min_z = 0.0f;
        float max_x = 0.0f, max_y = 0.0f, max_z = 0.0f;
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            auto vertex_pos = mesh->mVertices[i];
            
            if(vertex_pos.x < min_x)
                min_x = vertex_pos.x;
            else if(vertex_pos.x > max_x)
                max_x = vertex_pos.x;
            
            if(vertex_pos.y < min_y)
                min_y = vertex_pos.y;
            else if(vertex_pos.y > max_y)
                max_y = vertex_pos.y;
            
            if(vertex_pos.z < min_z)
                min_z = vertex_pos.z;
            else if(vertex_pos.z > max_z)
                max_z = vertex_pos.z;
        }
        
        AABB aabb;
        aabb.min = Vector3(min_x, min_y, min_z);
        aabb.max = Vector3(max_x, max_y, max_z);
        fwrite(&aabb, sizeof(AABB), 1, file);

        fwrite(&mesh->mNumVertices, sizeof(int), 1, file);

        unsigned int numIndices = mesh->mNumFaces * 3;
        fwrite(&numIndices, sizeof(int), 1, file);
        
        int numOffsetMatrices = mesh->mNumBones;
        fwrite(&numOffsetMatrices, sizeof(int), 1, file);
        
        for(int b = 0; b < mesh->mNumBones; b++) {
            auto offset = mesh->mBones[b]->mOffsetMatrix;
            offset.Transpose();
            
            fwrite(&offset, sizeof(aiMatrix4x4), 1, file);
        }

        int material_override = matNameToIndex[meshToMaterial[i]];
        fwrite(&material_override, sizeof(int), 1, file);
    }

    fclose(file);

    if(flags.export_materials) {
        for(int i = 0; i < sc->mNumMaterials; i++) {
            auto path = data_path + "/materials/" + sc->mMaterials[i]->GetName().C_Str() + ".material";

            // skip materials that already exist
            std::ifstream infile(path);
            if(infile.good())
                continue;

            nlohmann::json j;

            j["version"] = 1;

            aiColor4D color;
            aiGetMaterialColor(sc->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE,&color);

            j["baseColor"] = Vector3(color.r, color.g, color.b);
            j["metallic"] = 0.0f;
            j["roughness"] = 0.5f;

            std::ofstream out(path);
            out << j;
        }
    }

    if(flags.export_animations) {
        for(auto i = 0; i < sc->mNumAnimations; i++) {
            const aiAnimation* animation = sc->mAnimations[i];
            std::string animName = animation->mName.C_Str();

            if(animName.length() == 0)
                animName = "default";

            std::string finalName = (name + animName + ".anim");

            std::replace(finalName.begin(), finalName.end(), '|', '_');

            FILE* file = fopen((data_path + "/animations/" + finalName).c_str(), "wb");

            fwrite(&animation->mDuration, sizeof(double), 1, file);
            fwrite(&animation->mTicksPerSecond, sizeof(double), 1, file);

            fwrite(&animation->mNumChannels, sizeof(unsigned int), 1, file);

            for(auto j = 0; j < animation->mNumChannels; j++) {
                const aiNodeAnim* channel = animation->mChannels[j];

                write_string(file, channel->mNodeName);

                fwrite(&channel->mNumPositionKeys, sizeof(unsigned int), 1, file);

                for(auto k = 0; k < channel->mNumPositionKeys; k++) {
                    PositionKeyFrame key;
                    key.value = Vector3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z);
                    key.time = channel->mPositionKeys[k].mTime;

                    fwrite(&key, sizeof(PositionKeyFrame), 1, file);
                }

                fwrite(&channel->mNumRotationKeys, sizeof(unsigned int), 1, file);

                for(auto k = 0; k < channel->mNumRotationKeys; k++) {
                    RotationKeyFrame key;
                    key.value = Quaternion(channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z, channel->mRotationKeys[k].mValue.w);
                    key.time = channel->mRotationKeys[k].mTime;

                    fwrite(&key, sizeof(RotationKeyFrame), 1, file);
                }

                fwrite(&channel->mNumScalingKeys, sizeof(unsigned int), 1, file);

                for(auto k = 0; k < channel->mNumScalingKeys; k++) {
                    ScaleKeyFrame key;
                    key.value = Vector3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z);
                    key.time = channel->mScalingKeys[k].mTime;

                    fwrite(&key, sizeof(ScaleKeyFrame), 1, file);
                }
            }

            fclose(file);
        }
    }
}

void ModelEditor::drawUI() {
    ImGui::Begin("mcompile", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(platform::get_window_size(0));

    if(ImGui::Button("Open model to compile...")) {
        platform::open_dialog(true, [this](std::string path) {
            model_path = path;
        });
    }

    if(ImGui::Button("Open data path...")) {
        platform::open_dialog(true, [this](std::string path) {
            data_path = path;
        }, true);
    }

    ImGui::Checkbox("Compile as static (remove transforms)", &flags.compile_static);
    ImGui::Checkbox("Export materials", &flags.export_materials);
    ImGui::Checkbox("Export animations", &flags.export_animations);

    if(!model_path.empty() && !data_path.empty()) {
        ImGui::Text("%s will be compiled for data path %s", model_path.c_str(), data_path.c_str());

        if(ImGui::Button("Compile"))
            compile_model();
    } else {
        ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "Please select a path first before compiling!");
    }

    ImGui::End();
}

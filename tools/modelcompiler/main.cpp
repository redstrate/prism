#include <cstdio>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <unordered_map>
#include <functional>

#include <fmt/printf.h>

void traverseNode(const aiScene* scene, aiNode* node, const aiBone* bone, std::string& parent) {
    for(int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if(child->mName == bone->mName) {
            parent = node->mName.C_Str();
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
            
            fmt::print("{} bone position: {} {} {}\n", child->mName.C_Str(), position.x, position.y, position.z);
            
            pos = position;
        } else {
            traverseNode(scene, child, bone, pos);
        }
    }
}

std::string findBoneParent(const aiScene* scene, const aiBone* bone) {
    std::string t = "none";
    
    traverseNode(scene, scene->mRootNode, bone, t);
    
    return t;
}

aiVector3D getBonePosition(const aiScene* scene, const aiBone* bone) {
    aiVector3D pos;
    
    traverseNode(scene, scene->mRootNode, bone, pos);
    
    return pos;
}

int main(int argc, char* argv[]) {
  if(argc < 2)
    return -1;

  Assimp::Importer importer;

  const aiScene* sc = importer.ReadFile(argv[1], aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

  FILE* file = fopen(argv[2], "wb");

    int version = 1;
    fwrite(&version, sizeof(int), 1, file);
    
  fwrite(&sc->mNumMeshes, sizeof(int), 1, file);

  for (unsigned int i = 0; i < sc->mNumMeshes; i++) {
    const aiMesh* mesh = sc->mMeshes[i];

    /*fwrite(&mesh->mName.length, sizeof(unsigned int), 1, file);
    fwrite(mesh->mName.C_Str(), sizeof(char) * mesh->mName.length, 1, file);*/

    fwrite(&mesh->mNumVertices, sizeof(int), 1, file);

    unsigned int numIndices = mesh->mNumFaces * 3;
    fwrite(&numIndices, sizeof(int), 1, file);

    fwrite(&mesh->mNumBones, sizeof(int), 1, file);
      
      int numMaterials = 0;
    fwrite(&numMaterials, sizeof(int), 1, file);
      
    bool hasInverse = false;
    if(mesh->mNumBones != 0) {
      hasInverse = true;
      fwrite(&hasInverse, sizeof(bool), 1, file);

      auto transform = sc->mRootNode->mTransformation;
      transform.Inverse();
      transform.Transpose();

      fwrite(&transform, sizeof(aiMatrix4x4), 1, file);
    } else {
      fwrite(&hasInverse, sizeof(bool), 1, file);
    }

    for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
        struct Vertex {
            aiVector3D position, normal;
            aiVector2D uv;
            
            int materialOffset = 0;
        } vertex;
        
      vertex.position = mesh->mVertices[v];
      vertex.normal = mesh->mNormals[v];

        fwrite(&vertex, sizeof(vertex), 1, file);
    }

    for (unsigned int e = 0; e < mesh->mNumFaces; e++) {
      const aiFace& face = mesh->mFaces[e];

      fwrite(face.mIndices, sizeof(uint32_t) * 3, 1, file);
    }

    for (unsigned int b = 0; b < mesh->mNumBones; b++) {
      const aiBone* aBone = mesh->mBones[b];

        {
            std::string name = aBone->mName.C_Str();

            auto len = name.length();
            fwrite(&len, sizeof(unsigned int), 1, file);
            fwrite(name.c_str(), sizeof(char) * len, 1, file);
        }

        {
            std::string name = findBoneParent(sc, aBone);

            auto len = name.length();
            fwrite(&len, sizeof(unsigned int), 1, file);
            fwrite(name.c_str(), sizeof(char) * len, 1, file);
        }

        auto offset = aBone->mOffsetMatrix;
        offset.Transpose();

        fwrite(&offset, sizeof(aiMatrix4x4), 1, file);
        
        auto pos = getBonePosition(sc, aBone);
        
        fwrite(&pos, sizeof(float) * 3, 1, file);

        fwrite(&aBone->mNumWeights, sizeof(int), 1, file);

        fwrite(aBone->mWeights, sizeof(aiVertexWeight) * aBone->mNumWeights, 1, file);
    }
  }

    int numMaterials = 0;
    fwrite(&numMaterials, sizeof(int), 1, file);

    fclose(file);

  /*if(sc->mNumAnimations != 0) {
    for(auto i = 0; i < sc->mNumAnimations; i++) {
      const aiAnimation* animation = sc->mAnimations[i];
      std::string animName = animation->mName.C_Str();

      if(animName.length() == 0)
        animName = "default";

      std::transform(animName.begin(), animName.end(), animName.begin(), [](char ch) {
        return ch == ' ' ? '_' : ch;
      });
      animName += ".anim";

      std::cout << "Warning: creating animation file " << animName << std::endl;

      FILE* file = fopen(animName.c_str(), "wb");

      fwrite(&animation->mDuration, sizeof(double), 1, file);
      fwrite(&animation->mTicksPerSecond, sizeof(double), 1, file);

      fwrite(&animation->mNumChannels, sizeof(unsigned int), 1, file);

      for(auto j = 0; j < animation->mNumChannels; j++) {
        const aiNodeAnim* channel = animation->mChannels[j];

        const auto referringType = AnimationNode::ReferringType::Bone;
        fwrite(&referringType, sizeof(AnimationNode::ReferringType), 1, file);

        std::string name = channel->mNodeName.C_Str();
        if(name.find("-node") != std::string::npos)
          name = name.substr(0, name.length() - 5);

        auto len = name.length();
        fwrite(&len, sizeof(size_t), 1, file);
        fwrite(name.data(), sizeof(char) * len, 1, file);

        fwrite(&channel->mNumPositionKeys, sizeof(unsigned int), 1, file);

        for(auto k = 0; k < channel->mNumPositionKeys; k++) {
          AnimationKey<Vector3> key;
          key.value = Vector3(channel->mPositionKeys[k].mValue.x, channel->mPositionKeys[k].mValue.y, channel->mPositionKeys[k].mValue.z);
          key.time = channel->mPositionKeys[k].mTime;

          fwrite(&key, sizeof(AnimationKey<Vector3>), 1, file);
        }

        fwrite(&channel->mNumRotationKeys, sizeof(unsigned int), 1, file);

        for(auto k = 0; k < channel->mNumRotationKeys; k++) {
          AnimationKey<Quaternion> key;
          key.value = Quaternion(channel->mRotationKeys[k].mValue.x, channel->mRotationKeys[k].mValue.y, channel->mRotationKeys[k].mValue.z, channel->mRotationKeys[k].mValue.w);
          key.time = channel->mRotationKeys[k].mTime;

          fwrite(&key, sizeof(AnimationKey<Quaternion>), 1, file);
        }

        fwrite(&channel->mNumScalingKeys, sizeof(unsigned int), 1, file);

        for(auto k = 0; k < channel->mNumScalingKeys; k++) {
          AnimationKey<Vector3> key;
          key.value = Vector3(channel->mScalingKeys[k].mValue.x, channel->mScalingKeys[k].mValue.y, channel->mScalingKeys[k].mValue.z);
          key.time = channel->mScalingKeys[k].mTime;

          fwrite(&key, sizeof(AnimationKey<Vector3>), 1, file);
        }

        aiNode* affectedNode = sc->mRootNode->FindNode(channel->mNodeName.C_Str());
        fwrite(&affectedNode->mNumChildren, sizeof(unsigned int), 1, file);

        for(auto k = 0; k < affectedNode->mNumChildren; k++) {
          const aiNode* child = affectedNode->mChildren[k];

          std::string name = child->mName.C_Str();
          if(name.find("-node") != std::string::npos)
            name = name.substr(0, name.length() - 5);

          auto len = name.length();
          fwrite(&len, sizeof(size_t), 1, file);
          fwrite(name.data(), sizeof(char) * len, 1, file);
        }
      }

      fclose(file);
    }
  }*/

  return 0;
}

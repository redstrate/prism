#pragma once

#include <memory>
#include <unordered_map>
#include <array>

#include "file.hpp"
#include "assetptr.hpp"
#include "asset_types.hpp"
#include "string_utils.hpp"

namespace std {
    template <>
    struct hash<prism::path> {
        std::size_t operator()(const prism::path& k) const {
            return (std::hash<std::string>()(k.string()));
        }
    };
}

template<typename T>
std::unique_ptr<T> load_asset(const prism::path p);

template<typename T>
bool can_load_asset(const prism::path p);

template<class AssetType>
using AssetStore = std::unordered_map<prism::path, std::unique_ptr<AssetType>>;

template<class... Assets>
class AssetPool : public AssetStore<Assets>... {
public:
    template<typename T>
    AssetPtr<T> add() {
        const auto p = prism::path();
        auto reference_block = get_reference_block(p);
                
        AssetStore<T>::try_emplace(p, std::make_unique<T>());
        
        return AssetPtr<T>(AssetStore<T>::at(p).get(), reference_block);
    }

    template<typename T>
    AssetPtr<T> get(const prism::path path) {
        return fetch<T>(path, get_reference_block(path));
    }
    
    template<typename T>
    std::vector<T*> get_all() {
        std::vector<T*> assets;
        for(auto iter = AssetStore<T>::begin(); iter != AssetStore<T>::end(); iter++) {
            auto& [p, asset] = *iter;
            
            assets.push_back(asset.get());
        }
        
        return assets;
    }
    
    template<typename T>
    AssetPtr<T> fetch(const prism::path path, ReferenceBlock* reference_block) {
        if(!AssetStore<T>::count(path))
            AssetStore<T>::try_emplace(path, load_asset<T>(path));
       
        return AssetPtr<T>(AssetStore<T>::at(path).get(), reference_block);
    }
    
    std::tuple<Asset*, ReferenceBlock*> load_asset_generic(const prism::path path) {
        Asset* asset = nullptr;
        ReferenceBlock* block = nullptr;
        
        (load_asset_generic<Assets>(path, asset, block), ...);
        
        return {asset, block};
    }
    
    void perform_cleanup() {
        for(auto iter = reference_blocks.begin(); iter != reference_blocks.end();) {
            auto& [path, block] = *iter;
            
            if(block->references == 0) {
                ((delete_asset<Assets>(path)), ...);
                
                iter = reference_blocks.erase(iter);
            } else {
                iter = std::next(iter);
            }
        }
    }
    
    std::unordered_map<prism::path, std::unique_ptr<ReferenceBlock>> reference_blocks;
    
private:
    ReferenceBlock* get_reference_block(const prism::path path) {
        if(!reference_blocks.count(path))
            reference_blocks.try_emplace(path, std::make_unique<ReferenceBlock>());

        return reference_blocks[path].get();
    }
    
    template<typename T>
    void load_asset_generic(const prism::path path, Asset*& at, ReferenceBlock*& block) {
        if(can_load_asset<T>(path)) {
            if(!AssetStore<T>::count(path))
                AssetStore<T>::try_emplace(path, load_asset<T>(path));
            
            at = AssetStore<T>::at(path).get();
            block = get_reference_block(path);
        }
    }
    
    template<typename T>
    void delete_asset(const prism::path path) {
        auto iter = AssetStore<T>::find(path);
        if(iter != AssetStore<T>::end()) {
            auto& [_, asset] = *iter;
            
            asset.reset();
            
            AssetStore<T>::erase(iter);
        }
    }
};

using AssetManager = AssetPool<Mesh, Material, Texture>;

inline std::unique_ptr<AssetManager> assetm;

std::unique_ptr<Mesh> load_mesh(const prism::path path);
std::unique_ptr<Material> load_material(const prism::path path);
std::unique_ptr<Texture> load_texture(const prism::path path);

void save_material(Material* material, const prism::path path);

template<typename T>
std::unique_ptr<T> load_asset(const prism::path path) {
    if constexpr (std::is_same_v<T, Mesh>) {
        return load_mesh(path);
    } else if constexpr(std::is_same_v<T, Material>) {
        return load_material(path);
    } else if constexpr(std::is_same_v<T, Texture>) {
        return load_texture(path);
    }
}

template<typename T>
bool can_load_asset(const prism::path path) {
    if constexpr(std::is_same_v<T, Mesh>) {
        return path.extension() == ".model";
    } else if constexpr(std::is_same_v<T, Material>) {
        return path.extension() == ".material";
    } else if constexpr(std::is_same_v<T, Texture>) {
        return path.extension() == ".png";
    }
}

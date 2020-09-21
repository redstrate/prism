#pragma once

#include <memory>
#include <string>

struct ReferenceBlock {
    uint64_t references = 0;
};

class Asset {
public:
    std::string path;
};

template<class T>
struct AssetPtr {
    AssetPtr() {}

    AssetPtr(T* ptr, ReferenceBlock* block) : handle(ptr), block(block) {
        block->references++;
    }

    AssetPtr(const AssetPtr &rhs) {
        handle = rhs.handle;
        block = rhs.block;

        if(block != nullptr)
            block->references++;
    }

    AssetPtr& operator=(const AssetPtr& rhs) {
        handle = rhs.handle;
        block = rhs.block;

        if(block != nullptr)
            block->references++;

        return *this;
    }

    ~AssetPtr() {
        if(block != nullptr)
            block->references--;
    }
    
    void clear() {
        if(block != nullptr)
            block->references--;
        
        block = nullptr;
        handle = nullptr;
    }

    T* handle = nullptr;
    ReferenceBlock* block = nullptr;

    operator bool() const{
        return handle != nullptr;
    }

    T* operator->() {
        return handle;
    }

    T* operator->() const {
        return handle;
    }
    
    T* operator*() {
        return handle;
    }

    T* operator*() const {
        return handle;
    }
};

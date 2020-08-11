#pragma once

#include <Metal/Metal.h>

#include "gfx_texture.hpp"

class GFXMetalTexture : public GFXTexture {
public:
    id<MTLTexture> handle = nil;
    id<MTLSamplerState> sampler = nil;

    int array_length = 1;
    bool is_cubemap = false;
    
    MTLPixelFormat format;
};

#pragma once

#include <Metal/Metal.h>

#include "gfx_buffer.hpp"

class GFXMetalBuffer : public GFXBuffer {
public:
    id<MTLBuffer> handles[3] = {nil, nil, nil};
    bool dynamicData = false;
    
    id<MTLBuffer> get(int frameIndex) {
        if(dynamicData) {
            return handles[frameIndex];
        } else {
            return handles[0];
        }
    }
};

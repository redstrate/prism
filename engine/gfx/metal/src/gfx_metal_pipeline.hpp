#pragma once

#include <Metal/Metal.h>

#include "gfx_pipeline.hpp"

class GFXMetalPipeline : public GFXPipeline {
public:
    std::string label;
    
    id<MTLRenderPipelineState> handle = nil;
    id<MTLDepthStencilState> depthStencil = nil;
    MTLPrimitiveType primitiveType;

    MTLCullMode cullMode;
    GFXWindingMode winding_mode;

    struct VertexStride {
        int location, stride;
    };
    
    std::vector<VertexStride> vertexStrides;
    
    int pushConstantSize = 0;
    int pushConstantIndex = 0;
    
    bool renderWire = false;
};

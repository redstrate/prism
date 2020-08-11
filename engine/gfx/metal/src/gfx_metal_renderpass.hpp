#pragma once

#include <Metal/Metal.h>

#include "gfx_renderpass.hpp"

class GFXMetalRenderPass : public GFXRenderPass {
public:
    std::vector<MTLPixelFormat> attachments;
};

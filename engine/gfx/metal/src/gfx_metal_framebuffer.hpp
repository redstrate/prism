#pragma once

#include <Metal/Metal.h>
#include <vector>

#include "gfx_framebuffer.hpp"

class GFXMetalTexture;

class GFXMetalFramebuffer : public GFXFramebuffer {
public:
    std::vector<GFXMetalTexture*> attachments;
};

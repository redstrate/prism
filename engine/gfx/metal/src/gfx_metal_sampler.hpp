#pragma once

#include <Metal/Metal.h>

#include "gfx_sampler.hpp"

class GFXMetalSampler : public GFXSampler {
public:
    id<MTLSamplerState> handle = nil;
};

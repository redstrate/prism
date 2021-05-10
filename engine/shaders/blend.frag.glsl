#define SMAA_INCLUDE_PS 1
#include "smaa_common.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inOffset[3];
layout(location = 5) in vec2 inPixUV;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D edgeSampler;
layout(binding = 1) uniform sampler2D areaSampler;
layout(binding = 3) uniform sampler2D searchSampler;

void main() {
    outColor = SMAABlendingWeightCalculationPS(inUV, inPixUV, inOffset, edgeSampler, areaSampler, searchSampler, ivec4(0));
}

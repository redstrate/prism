#define SMAA_INCLUDE_PS 1
#include "smaa_common.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inOffset[3];

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D imageSampler;
layout(binding = 1) uniform sampler2D depthSampler;

void main() {
    vec2 edge = SMAALumaEdgeDetectionPS(inUV, inOffset, imageSampler, depthSampler);

    outColor = vec4(edge, 0.0, 1.0);
}

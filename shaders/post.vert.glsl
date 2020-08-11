#define SMAA_RT_METRICS viewport

#define SMAA_PRESET_ULTRA 1
#define SMAA_GLSL_4 1
#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0

layout(std430, push_constant, binding = 4) uniform readonlPushConstant {
    vec4 viewport;
    float fade;
    bool performGammaCorrection;
    bool performAA;
};

#include "smaa.glsl"

layout (location = 0) out vec2 outUV;
layout(location = 1) out vec4 outOffset;

void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
    
    SMAANeighborhoodBlendingVS(outUV, outOffset);
}

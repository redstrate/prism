#define SMAA_RT_METRICS viewport

#define SMAA_PRESET_ULTRA 1
#define SMAA_GLSL_4 1
#define SMAA_PREDICATION 1

layout(std430, push_constant, binding = 2) uniform PushConstant {
    vec4 viewport;
    mat4 correctionMatrix;
};

#include "smaa.glsl"

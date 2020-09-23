#include "common.nocompile.glsl"

layout(local_size_x = 256, local_size_y = 1) in;

layout(r16f, binding = 0) uniform image2D target_image;

// adapated from https://bruop.github.io/exposure/ and http://www.alextardif.com/HistogramLuminance.html

shared uint histogram_shared[256];

layout(std430, binding = 1) buffer HistogramBuffer {
    uint histogram[];
};

layout(push_constant, binding = 2) uniform readonly PushConstant{
    vec4 params;
};

void main() {
    uint count_for_this_bin = histogram[gl_LocalInvocationIndex];
    histogram_shared[gl_LocalInvocationIndex] = count_for_this_bin * gl_LocalInvocationIndex;
    
    groupMemoryBarrier();
    
    histogram[gl_LocalInvocationIndex] = 0;
    
    for(uint cutoff = (256 >> 1); cutoff > 0; cutoff >>= 1) {
        if(uint(gl_LocalInvocationIndex) < cutoff) {
            histogram_shared[gl_LocalInvocationIndex] += histogram_shared[gl_LocalInvocationIndex + cutoff];
        }
        
        groupMemoryBarrier();
    }
    
    if(gl_LocalInvocationIndex == 0) {
        float weightedLogAverage = (histogram_shared[0] / max(params.w - float(count_for_this_bin), 1.0)) - 1.0;
        
        float weightedAvgLum = exp2(weightedLogAverage / 254.0 * params.y + params.x);

        float lumLastFrame = imageLoad(target_image, ivec2(0, 0)).x;
        float adaptedLum = lumLastFrame + (weightedAvgLum - lumLastFrame) * params.z;
        imageStore(target_image, ivec2(0, 0), vec4(adaptedLum, 0.0, 0.0, 0.0));
    }
}

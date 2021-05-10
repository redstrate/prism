#include "common.nocompile.glsl"

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D hdr_image;

// adapated from https://bruop.github.io/exposure/ and http://www.alextardif.com/HistogramLuminance.html

// Taken from RTR vol 4 pg. 278
#define RGB_TO_LUM vec3(0.2125, 0.7154, 0.0721)

shared uint histogram_shared[256];

layout(std430, binding = 1) buffer HistogramBuffer {
    uint histogram[];
};

layout(push_constant, binding = 2) uniform readonly PushConstant{
    vec4 params;
};

uint color_to_bin(const vec3 hdr_color, const float min_log_lum, const float inverse_log_lum_range) {
    const float lum = dot(hdr_color, RGB_TO_LUM);
    
    if (lum < EPSILON) {
        return 0;
    }
    
    const float log_lum = clamp((log2(lum) - min_log_lum) * inverse_log_lum_range, 0.0, 1.0);
    
    return uint(log_lum * 254.0 + 1.0);
}

void main() {
    histogram_shared[gl_LocalInvocationIndex] = 0;
    groupMemoryBarrier();
    
    uvec2 dim = uvec2(params.zw);
    if(gl_GlobalInvocationID.x < dim.x && gl_GlobalInvocationID.y < dim.y) {
        vec3 hdr_color = imageLoad(hdr_image, ivec2(gl_GlobalInvocationID.xy)).rgb;
        uint bin_index = color_to_bin(hdr_color, params.x, params.y);
        
        atomicAdd(histogram_shared[bin_index], 1);
    }
    
    groupMemoryBarrier();
    
    atomicAdd(histogram[gl_LocalInvocationIndex], histogram_shared[gl_LocalInvocationIndex]);
}

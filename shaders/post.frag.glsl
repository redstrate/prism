#define SMAA_RT_METRICS viewport

#define SMAA_PRESET_ULTRA 1
#define SMAA_GLSL_4 1
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

layout(std430, push_constant, binding = 4) uniform PushConstant {
    vec4 viewport;
    vec4 options;
};

#include "smaa.glsl"
#include "common.nocompile.glsl"

layout (location = 0) in vec2 inUV;
layout(location = 1) in vec4 inOffset;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D colorSampler;
layout (binding = 2) uniform sampler2D backSampler;
layout (binding = 3) uniform sampler2D blendSampler;
layout (binding = 4) uniform sampler2D sobelSampler;

float calculate_sobel() {
    float x = 1.0 / viewport.z;
    float y = 1.0 / viewport.w;
    vec4 horizEdge = vec4( 0.0 );
    horizEdge -= texture( sobelSampler, vec2( inUV.x - x, inUV.y - y ) ) * 1.0;
    horizEdge -= texture( sobelSampler, vec2( inUV.x - x, inUV.y     ) ) * 2.0;
    horizEdge -= texture( sobelSampler, vec2( inUV.x - x, inUV.y + y ) ) * 1.0;
    horizEdge += texture( sobelSampler, vec2( inUV.x + x, inUV.y - y ) ) * 1.0;
    horizEdge += texture( sobelSampler, vec2( inUV.x + x, inUV.y     ) ) * 2.0;
    horizEdge += texture( sobelSampler, vec2( inUV.x + x, inUV.y + y ) ) * 1.0;
    vec4 vertEdge = vec4( 0.0 );
    vertEdge -= texture( sobelSampler, vec2( inUV.x - x, inUV.y - y ) ) * 1.0;
    vertEdge -= texture( sobelSampler, vec2( inUV.x    , inUV.y - y ) ) * 2.0;
    vertEdge -= texture( sobelSampler, vec2( inUV.x + x, inUV.y - y ) ) * 1.0;
    vertEdge += texture( sobelSampler, vec2( inUV.x - x, inUV.y + y ) ) * 1.0;
    vertEdge += texture( sobelSampler, vec2( inUV.x    , inUV.y + y ) ) * 2.0;
    vertEdge += texture( sobelSampler, vec2( inUV.x + x, inUV.y + y ) ) * 1.0;
    return sqrt((horizEdge.rgb * horizEdge.rgb) + (vertEdge.rgb * vertEdge.rgb)).r;
}

void main() {
    // passthrough
    if(options.w == 1) {
        outColor = texture(colorSampler, inUV);
        return;
    }
    
    vec3 sceneColor = vec3(0);
    if(options.x == 1) // enable AA
        sceneColor = SMAANeighborhoodBlendingPS(inUV, inOffset, colorSampler, blendSampler).rgb;
    else
        sceneColor = texture(colorSampler, inUV).rgb;
    
    float sobel = 0.0;
    if(textureSize(sobelSampler, 0).x > 1)
        sobel = calculate_sobel();
    
    vec3 sobelColor = vec3(0, 1, 1);
    
    vec3 hdrColor = sceneColor; // fading removed
    hdrColor = vec3(1.0) - exp(-hdrColor * options.z); // exposure
    outColor = vec4(from_linear_to_srgb(hdrColor) + (sobelColor * sobel), 1.0);
}

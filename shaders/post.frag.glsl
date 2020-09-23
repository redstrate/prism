#define SMAA_RT_METRICS viewport

#define SMAA_PRESET_ULTRA 1
#define SMAA_GLSL_4 1
#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1

layout(std430, push_constant, binding = 4) uniform PushConstant {
    vec4 viewport;
    vec4 options;
    vec4 transform_ops;
};

#include "smaa.glsl"
#include "common.nocompile.glsl"

layout (location = 0) in vec2 inUV;
layout(location = 1) in vec4 inOffset;

layout (location = 0) out vec4 outColor;

layout (binding = 1) uniform sampler2D colorSampler;
layout (binding = 2) uniform sampler2D backSampler;
layout (binding = 3) uniform sampler2D blendSampler;
layout (binding = 5) uniform sampler2D sobelSampler;
layout (binding = 6) uniform sampler2D averageLuminanceSampler;
layout (binding = 7) uniform sampler2D farFieldSampler;

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

// adapted from https://bruop.github.io/exposure/

vec3 convertRGB2XYZ(vec3 _rgb)
{
    // Reference:
    // RGB/XYZ Matrices
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    vec3 xyz;
    xyz.x = dot(vec3(0.4124564, 0.3575761, 0.1804375), _rgb);
    xyz.y = dot(vec3(0.2126729, 0.7151522, 0.0721750), _rgb);
    xyz.z = dot(vec3(0.0193339, 0.1191920, 0.9503041), _rgb);
    return xyz;
}

vec3 convertXYZ2RGB(vec3 _xyz)
{
    vec3 rgb;
    rgb.x = dot(vec3( 3.2404542, -1.5371385, -0.4985314), _xyz);
    rgb.y = dot(vec3(-0.9692660,  1.8760108,  0.0415560), _xyz);
    rgb.z = dot(vec3( 0.0556434, -0.2040259,  1.0572252), _xyz);
    return rgb;
}

vec3 convertXYZ2Yxy(vec3 _xyz)
{
    // Reference:
    // http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
    float inv = 1.0/dot(_xyz, vec3(1.0, 1.0, 1.0) );
    return vec3(_xyz.y, _xyz.x*inv, _xyz.y*inv);
}

vec3 convertYxy2XYZ(vec3 _Yxy)
{
    // Reference:
    // http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
    vec3 xyz;
    xyz.x = _Yxy.x*_Yxy.y/_Yxy.z;
    xyz.y = _Yxy.x;
    xyz.z = _Yxy.x*(1.0 - _Yxy.y - _Yxy.z)/_Yxy.z;
    return xyz;
}

vec3 convertRGB2Yxy(vec3 _rgb)
{
    return convertXYZ2Yxy(convertRGB2XYZ(_rgb) );
}

vec3 convertYxy2RGB(vec3 _Yxy)
{
    return convertXYZ2RGB(convertYxy2XYZ(_Yxy) );
}

float reinhard2(float _x, float _whiteSqr)
{
    return (_x * (1.0 + _x/_whiteSqr) ) / (1.0 + _x);
}

void main() {
    // passthrough
    if(options.w == 1) {
        outColor = texture(colorSampler, inUV);
        return;
    }
    
    bool enable_dof = options.w == 2;
    vec3 sceneColor = vec3(0);
    if(enable_dof) {
        sceneColor = texture(farFieldSampler, inUV).rgb;
        sceneColor += texture(colorSampler, inUV).rgb;
    } else {
        if(options.x == 1) // enable AA
            sceneColor = SMAANeighborhoodBlendingPS(inUV, inOffset, colorSampler, blendSampler).rgb;
        else
            sceneColor = texture(colorSampler, inUV).rgb;
    }

    float sobel = 0.0;
    if(textureSize(sobelSampler, 0).x > 1)
        sobel = calculate_sobel();
    
    vec3 sobelColor = vec3(0, 1, 1);
    
    vec3 hdrColor = sceneColor; // fading removed
    
    float avg_lum = texture(averageLuminanceSampler, inUV).r;
    
    vec3 transformed_color = hdrColor;

    if(transform_ops.y == 1) {
        transformed_color = vec3(1.0) - exp(-transformed_color * options.z);
    } else if(transform_ops.y == 2) {
        vec3 Yxy = convertRGB2Yxy(transformed_color);
        
        // hard-coded for now
        float whitePoint = 4.0;
        
        float lp = Yxy.x / (9.6 * avg_lum + 0.0001);
        
        // Replace this line with other tone mapping functions
        // Here we applying the curve to the luminance exclusively
        Yxy.x = reinhard2(lp, whitePoint);
        
        transformed_color = convertYxy2RGB(Yxy);
    }
    
    if(transform_ops.x == 1) {
        transformed_color = from_linear_to_srgb(transformed_color);
    }
    
    outColor = vec4(transformed_color + (sobelColor * sobel), 1.0);
}

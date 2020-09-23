layout(location = 0) in vec2 inUV;
layout(location = 1) in flat ivec2 inPixel;
layout(location = 2) in float depth;

layout(location = 0) out vec4 outColor;

layout(rgba32f, binding = 0) uniform image2D color_sampler;
layout(binding = 3) uniform sampler2D aperture_sampler;

layout(push_constant, binding = 2) uniform readonly PushConstant{
    vec4 params;
};

void main() {
    // far field mode
    if(params.y == 0) {
        if(depth < 0.98)
            discard;
    }
    
    if(inUV.y > 1.0 || inUV.x > 1.0)
        discard;
    
    outColor = vec4(imageLoad(color_sampler, inPixel).rgb, 1.0);
    if(params.y == 0) {
        outColor = outColor * texture(aperture_sampler, inUV);
    }
}

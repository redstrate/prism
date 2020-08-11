layout(location = 0) in vec2 inUV;
layout(location = 1) in flat ivec2 inPixel;
layout(location = 2) in float depth;

layout(location = 0) out vec4 outColor;

layout(rgba32f, binding = 0) uniform image2D color_sampler;
layout(binding = 2) uniform sampler2D aperture_sampler;

vec4 fromLinear(vec4 linearRGB)
{
    return pow(linearRGB, vec4(1.0/2.2));
}

void main() {
    /*if(depth < 0.98)
        discard;*/
    if(inUV.y > 1.0 || inUV.x > 1.0)
        discard;
    
    outColor = vec4(fromLinear(imageLoad(color_sampler, inPixel)).rgb, 1.0) * texture(aperture_sampler, inUV);
}

layout(location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (binding = 3) uniform sampler2D fontSampler;

vec4 toSRGB(vec4 v) {
    return vec4(pow(v.rgb, vec3(2.2)), v.a);
}

void main() {
    outColor = vec4(vec3(texture(fontSampler, inUV).r), texture(fontSampler, inUV).r);
    
    outColor = toSRGB(outColor);
}

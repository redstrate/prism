layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(push_constant, binding = 1) uniform readonly PushConstant{
    mat4 mvp;
    vec4 color;
};

layout (binding = 2) uniform sampler2D colorSampler;

void main() {
    if(inUV.y < 0.0 || inUV.x > 1.0)
        discard;
    
    outColor = texture(colorSampler, inUV) * color;
}

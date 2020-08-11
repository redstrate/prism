layout(location = 0) out vec4 outColor;

layout(push_constant, binding = 1) uniform PushConstant {
    mat4 mvp;
    vec4 color;
};

void main() {
    outColor = color;
}

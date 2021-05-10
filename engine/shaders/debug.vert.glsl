layout (location = 0) in vec3 inPosition;

layout(push_constant, binding = 1) uniform PushConstant{
    mat4 mvp;
    vec4 color;
};

void main() {
    gl_Position = mvp * vec4(inPosition, 1.0);
}

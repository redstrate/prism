layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_uv;

layout(std430, push_constant, binding = 1) uniform PushConstant {
    mat4 projection;
};

void main() {
    gl_Position = projection * vec4(in_position, 0.0, 1.0);
    out_color = in_color;
    out_uv = in_uv;
}

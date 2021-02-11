layout (location = 0) out vec2 out_uv;

layout(push_constant, binding = 1) uniform PushConstant{
    mat4 view;
    vec4 sun_position_fov;
    float aspect;
};

void main() {
    out_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(out_uv * 2.0f + -1.0f, 1.0f, 1.0f);
}

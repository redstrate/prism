layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 2) uniform sampler2D bound_texture;

void main() {
    out_color = in_color * texture(bound_texture, in_uv);
}

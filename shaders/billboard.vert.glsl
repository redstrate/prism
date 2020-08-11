layout (location = 0) out vec2 outUV;

layout(push_constant, binding = 1) uniform readonly PushConstant{
    mat4 view, mvp;
    vec4 color;
};

void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outUV = vec2(p.x, 1.0 - p.y);
    
    p = p * 2.0f + -1.0f;

    const vec3 right = {view[0][0], view[1][0], view[2][0]};
    const vec3 up = {view[0][1], view[1][1], view[2][1]};
    
    vec3 position = right * p.x * 0.25 + up * p.y * 0.25;
    
    gl_Position = mvp * vec4(position, 1.0);
}
    

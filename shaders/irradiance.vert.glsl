layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outPos;

layout(push_constant, binding = 1) uniform readonly PushConstant{
    mat4 mvp;
};

void main() {
    gl_Position =  mvp * vec4(inPosition, 1.0);
    outPos = inPosition;
}

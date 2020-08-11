layout(location = 0) in vec3 inPos;

layout (location = 0) out float outFragColor;

layout(push_constant, binding = 0) uniform PushConstant {
    mat4 mvp, model;
    vec3 lightPos;
};

void main() {
    outFragColor = length(inPos - lightPos);
}

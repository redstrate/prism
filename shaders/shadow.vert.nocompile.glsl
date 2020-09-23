layout (constant_id = 0) const int max_lights = 25;

layout (location = 0) in vec3 inPosition;

#ifdef BONE
layout (location = 4) in ivec4 inBoneID;
layout (location = 5) in vec4 inBoneWeight;
#endif

layout (location = 0) out vec3 outPos;
layout (location = 1) flat out int index;

layout(push_constant, binding = 0) uniform PushConstant {
    mat4 mvp, model;
};

#ifdef BONE
layout(std430, binding = 1) buffer readonly BoneInformation {
    mat4 bones[128];
};
#endif

layout(std430, binding = 2) buffer readonly LightInformation {
    vec3 light_locations[max_lights];
};

void main() {
#ifdef BONE
    mat4 BoneTransform;

    BoneTransform = bones[inBoneID[0]] * inBoneWeight[0];
    BoneTransform += bones[inBoneID[1]] * inBoneWeight[1];
    BoneTransform += bones[inBoneID[2]] * inBoneWeight[2];
    BoneTransform += bones[inBoneID[3]] * inBoneWeight[3];
    
    gl_Position = mvp * BoneTransform * vec4(inPosition, 1.0);
    outPos = vec3(model * vec4(inPosition, 1.0));
#else
    gl_Position =  mvp * vec4(inPosition, 1.0);
    outPos = vec3(model * vec4(inPosition, 1.0));    
#endif
    index = gl_BaseInstance;
}

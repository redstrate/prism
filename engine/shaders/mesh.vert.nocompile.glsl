layout (constant_id = 0) const int max_materials = 25;
layout (constant_id = 1) const int max_lights = 25;
layout (constant_id = 2) const int max_spot_lights = 4;
layout (constant_id = 3) const int max_probes = 4;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

#ifdef BONE
layout (location = 5) in ivec4 inBoneID;
layout (location = 6) in vec4 inBoneWeight;
#endif

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 4) out vec4 fragPosLightSpace;
layout (location = 5) out mat3 outTBN;
layout (location = 14) out vec4 fragPostSpotLightSpace[max_spot_lights];

struct Material {
    vec4 color, info;
};

struct Light {
    vec4 positionType;
    vec4 directionPower;
    vec4 color;
};

struct Probe {
    vec4 position, size;
};

layout(std430, binding = 1) buffer readonly SceneInformation {
    vec4 options;
    vec4 camPos;
    mat4 vp, lightSpace;
    mat4 spotLightSpaces[max_spot_lights];
    Material materials[max_materials];
    Light lights[max_lights];
    Probe probes[max_probes];
    int numLights;
} scene;

#ifdef CUBEMAP
layout(push_constant, binding = 0) uniform readonly PushConstant{
    mat4 model, view;
};
#else
layout(push_constant, binding = 0) uniform readonly PushConstant{
    mat4 model;
};
#endif

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

#ifdef BONE
layout(std430, binding = 14) buffer readonly BoneInformation {
    mat4 bones[128];
};
#endif

void main() {
    const mat3 mat = mat3(model);
    const vec3 T = normalize(mat * inTangent);
    const vec3 N = normalize(mat * inNormal);
    const vec3 B = normalize(mat * inBitangent);
    const mat3 TBN = mat3(T, B, N);
    
#ifdef BONE
    mat4 BoneTransform = bones[inBoneID[0]] * inBoneWeight[0];
    BoneTransform += bones[inBoneID[1]] * inBoneWeight[1];
    BoneTransform += bones[inBoneID[2]] * inBoneWeight[2];
    BoneTransform += bones[inBoneID[3]] * inBoneWeight[3];
    
    BoneTransform = model * BoneTransform;
    
    vec4 bPos = BoneTransform * vec4(inPosition, 1.0);
    vec4 bNor = BoneTransform * vec4(inNormal, 0.0);
    
    gl_Position = scene.vp * bPos;
    outFragPos = vec3(model * vec4(inPosition, 1.0));
    outNormal = bNor.xyz;
    outUV = inUV;
    fragPosLightSpace = (biasMat * scene.lightSpace) * bPos;
    
    for(int i = 0; i < max_spot_lights; i++) {
        fragPostSpotLightSpace[i] = (biasMat * scene.spotLightSpaces[i]) * bPos;
    }
#else
#ifdef CUBEMAP
    gl_Position = scene.vp * view * model * vec4(inPosition, 1.0);
    outFragPos = vec3(model * vec4(inPosition, 1.0));
    outNormal = mat3(model) * inNormal;
    outUV = inUV;
    fragPosLightSpace = (biasMat * scene.lightSpace * model) * vec4(inPosition, 1.0);
    
    for(int i = 0; i < max_spot_lights; i++) {
        fragPostSpotLightSpace[i] = (biasMat * scene.spotLightSpaces[i] * model) * vec4(inPosition, 1.0);
    }
#else
    gl_Position = scene.vp * model * vec4(inPosition, 1.0);
    
    outFragPos = vec3(model * vec4(inPosition, 1.0));
    outNormal = mat3(model) * inNormal;
    outUV = inUV;
    fragPosLightSpace = (biasMat * scene.lightSpace * model) * vec4(inPosition, 1.0);
    
    for(int i = 0; i < max_spot_lights; i++) {
        fragPostSpotLightSpace[i] = (biasMat * scene.spotLightSpaces[i] * model) * vec4(inPosition, 1.0);
    }
#endif
#endif
    outTBN = TBN;
}

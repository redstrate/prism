layout (constant_id = 0) const int max_materials = 25;
layout (constant_id = 1) const int max_lights = 25;
layout (constant_id = 2) const int max_spot_lights = 4;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out flat int outMaterialId;
layout (location = 4) out vec4 fragPosLightSpace;
layout (location = 5) out vec4 fragPostSpotLightSpace[max_spot_lights];

struct Material {
    vec4 color, info;
};

struct Light {
    vec4 positionType;
    vec4 directionPower;
    vec4 color;
};

layout(std430, binding = 5) buffer readonly SceneInformation {
    vec4 camPos;
    mat4 projection, lightSpace;
    vec4 skyColor;
    mat4 spotLightSpaces[max_spot_lights];
    Material materials[max_materials];
    Light lights[max_lights];
    int numLights;
} scene;

layout(push_constant, binding = 6) uniform readonly PushConstant{
    mat4 model, view;
    int materialOffset;
};

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

void main() {
    gl_Position = scene.projection * view * model * vec4(inPosition, 1.0);
    outFragPos = vec3(model * vec4(inPosition, 1.0));
    outNormal = mat3(model) * inNormal;
    outUV = inUV;
    outMaterialId = materialOffset;
    fragPosLightSpace = (biasMat * scene.lightSpace * model) * vec4(inPosition, 1.0);
    
    for(int i = 0; i < max_spot_lights; i++) {
        fragPostSpotLightSpace[i] = (biasMat * scene.spotLightSpaces[i] * model) * vec4(inPosition, 1.0);
    }
}

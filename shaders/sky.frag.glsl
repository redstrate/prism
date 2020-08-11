#extension GL_GOOGLE_include_directive : enable

#include "atmosphere.glsl"

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(push_constant, binding = 1) uniform PushConstant{
    mat4 view;
    vec4 aspectFovY, sunPosition;
};

vec3 skyRay (vec2 Texcoord) {
    float d = 0.5 / tan(aspectFovY.y/2.0);
    return normalize(vec3((Texcoord.x - 0.5) * aspectFovY.x,
                           Texcoord.y - 0.5,
                           -d));
}

void main() {
    vec3 vPosition = mat3(view) * skyRay(inUV);

    vec3 color = atmosphere(
        normalize(vPosition),           // normalized ray direction
        vec3(0,6372e3,0),               // ray origin
        sunPosition.xyz,                        // position of the sun
        22.0,                           // intensity of the sun
        6371e3,                         // radius of the planet in meters
        6471e3,                         // radius of the atmosphere in meters
        vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
        21e-6,                          // Mie scattering coefficient
        8e3,                            // Rayleigh scale height
        1.2e3,                          // Mie scale height
        0.758                           // Mie preferred scattering direction
    );

    // Apply exposure.
    color = 1.0 - exp(-1.0 * color);

    outColor = vec4(color, 1);
}

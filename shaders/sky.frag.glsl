#include "atmosphere.glsl"

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout(push_constant, binding = 1) uniform PushConstant{
    mat4 view;
    vec4 sun_position_fov;
    float aspect;
};

vec3 sky_ray(const vec2 uv) {
    const float d = 0.5 / tan(sun_position_fov.w / 2.0);
    return normalize(vec3((uv.x - 0.5) * aspect,
                           uv.y - 0.5,
                           d));
}

void main() {
    const vec3 color = atmosphere(
        // ray direction
        normalize(mat3(view) * sky_ray(in_uv)),
        // ray origin
        vec3(0, 6372e3, 0),
        // position of the sun in world space (this will be normalized)
        sun_position_fov.xyz,
        // intensity of the sun
        22.0,
        // radius of the plant in meters
        6371e3,
        // radius of the atmosphere in meters
        6471e3,
        // Rayleigh scattering coefficient
        vec3(5.5e-6, 13.0e-6, 22.4e-6),
        // Mie scattering coefficient
        21e-6,
        // Rayleigh scale height
        8e3,
        // Mie scale height
        1.2e3,
        // Mie preferred scattering direction
        0.758
    );

    // apply exposure
    out_color = vec4(1.0 - exp(-1.0 * color), 1.0);
}

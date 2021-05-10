layout (constant_id = 0) const int max_lights = 25;

layout (location = 0) in vec3 inPos;
layout (location = 1) flat in int index;

layout (location = 0) out float outFragColor;

layout(std430, binding = 2) buffer readonly LightInformation {
    vec3 light_locations[max_lights];
};

void main() {
    outFragColor = length(inPos - light_locations[index]);
}

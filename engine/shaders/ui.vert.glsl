#extension GL_GOOGLE_include_directive : require
#include "font.glsl"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;

struct ElementInstance {
    vec4 color;
   uint position, size;
   uint padding[2];
};

layout(std430, binding = 0) buffer readonly ElementBuffer {
  ElementInstance elements[];
};

layout(push_constant, binding = 1) uniform readonly PushConstant{
    vec2 screenSize;
    mat4 mvp;
};

void main() {
   const ElementInstance instance = elements[gl_InstanceIndex];

   vec2 p = vec2(gl_VertexIndex % 2, gl_VertexIndex / 2);
   outUV = p;

   p *= fixed2_to_vec2(instance.size);
   p += fixed2_to_vec2(instance.position);
   p *= vec2(1, -1);
   p *= 2.0 / screenSize;
   p += vec2(-1, 1);

   outColor = instance.color;

   gl_Position = mvp * vec4(p, 0.0, 1.0);
}
    

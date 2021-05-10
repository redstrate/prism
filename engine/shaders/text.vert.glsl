#extension GL_GOOGLE_include_directive : require
#include "font.glsl"

layout(location = 0) out vec2 outUV;

struct GlyphInstance {
    uint position, index, instance;
};

struct StringInstance {
    uint xy;
};

struct GlyphMetric {
    uint x0_y0, x1_y1;
    float xoff, yoff;
    float xoff2, yoff2;
};

layout(std430, binding = 0) buffer readonly InstanceBuffer {
    GlyphInstance instances[];
};

layout(std430, binding = 1) buffer readonly MetricBuffer {
    GlyphMetric metrics[];
};

layout(std430, binding = 2) buffer readonly StringBuffer {
    StringInstance strings[];
};

layout(push_constant, binding = 4) uniform readonly PushConstant{
    vec2 screenSize;
    mat4 mvp;
};

void main() {
    const GlyphInstance instance = instances[gl_InstanceIndex];
    const GlyphMetric metric = metrics[getLower(instance.index)];

    vec2 p = vec2(gl_VertexIndex % 2, gl_VertexIndex / 2);

    const vec2 p0 = uint_to_vec2(metric.x0_y0);
    const vec2 p1 = uint_to_vec2(metric.x1_y1);

    outUV = (p0 + (p1 - p0) * p) / vec2(2048, 1132);

    p *= vec2(metric.xoff2 - metric.xoff, metric.yoff2 - metric.yoff);
    p += vec2(metric.xoff, metric.yoff);
    p += fixed2_to_vec2(instance.position);
    p += vec2(0, 18.2316);

    const StringInstance string = strings[instance.instance];

    p += fixed2_to_vec2(string.xy);
    p *= vec2(1, -1);
    p *= 2.0 / screenSize;
    p += vec2(-1, 1);

    gl_Position = mvp * vec4(p, 0.0, 1.0);
}

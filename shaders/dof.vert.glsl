layout(constant_id = 0) const int width = 25;
layout(constant_id = 1) const int height = 25;

layout(location = 0) out vec2 outUV;
layout(location = 1) out flat ivec2 outPixel;
layout(location = 2) out float outDepth;

layout(binding = 1) uniform sampler2D depth_sampler;

void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    
    ivec2 pixel = ivec2(gl_InstanceIndex % width, gl_InstanceIndex / width);
    outPixel = pixel;
    
    const float depth = texture(depth_sampler, vec2(pixel) / vec2(width, height)).r;
    outDepth = depth;
    
    vec2 pos = vec2(outUV * 2.0 + -1.0);
    pos *= 5.0 * depth;
    pos += vec2(pixel.x, pixel.y);
    pos *= 2.0 / vec2(width, height);
    pos += vec2(-1, -1);
    
    gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);
}

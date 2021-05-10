#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#include "smaa_common.glsl"

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outOffset[3];

void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = correctionMatrix * vec4(outUV * 2.0 + -1.0, 0.0, 1.0);

    SMAAEdgeDetectionVS(outUV, outOffset);
}

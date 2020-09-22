layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D hdr_image;

shared uint histogram_shared[256];

void main() {
    histogram_shared[gl_LocalInvocationIndex] = 1;
    groupMemoryBarrier();
}

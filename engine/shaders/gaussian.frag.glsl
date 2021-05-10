layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout(push_constant, binding = 1) uniform PushConstants {
    int horizontal;
};

layout (binding = 0) uniform sampler2D image;

const float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, inUV).rgb * weight[0]; // current fragment's contribution
    if(horizontal == 1)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += max(texture(image, inUV + vec2(tex_offset.x * i, 0.0)).rgb * weight[i], vec3(0.0));
            result += max(texture(image, inUV - vec2(tex_offset.x * i, 0.0)).rgb * weight[i], vec3(0.0));
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += max(texture(image, inUV + vec2(0.0, tex_offset.y * i)).rgb * weight[i], vec3(0.0));
            result += max(texture(image, inUV - vec2(0.0, tex_offset.y * i)).rgb * weight[i], vec3(0.0));
        }
    }
    
    outColor = vec4(result, 1.0);
}

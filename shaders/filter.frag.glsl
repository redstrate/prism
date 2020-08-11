#include "common.nocompile.glsl"

layout (constant_id = 0) const int texture_size = 512;

layout(location = 0) in vec3 inPos;
layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform samplerCube environmentSampler;

layout(push_constant, binding = 1) uniform readonly PushConstant{
    mat4 mvp;
    float roughness;
};

void main() {
    const vec3 N = normalize(inPos);
    
    // make the simplyfying assumption that V equals R equals the normal
    const vec3 R = N;
    const vec3 V = R;
    
    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        const vec2 Xi = hammersley(i, SAMPLE_COUNT);
        const vec3 H = importance_sample_ggx(Xi, N, roughness);
        const vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0) {
            // sample from the environment's mip level based on roughness/pdf
            const float D = geometry_slick_direct(N, H, roughness);
            const float NdotH = max(dot(N, H), 0.0);
            const float HdotV = max(dot(H, V), 0.0);
            const float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
            
            const float saTexel = 4.0 * PI / (6.0 * texture_size * texture_size);
            const float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            
            const float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
            
            prefilteredColor += textureLod(environmentSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
        
    outColor = vec4(prefilteredColor / totalWeight, 1.0);
}

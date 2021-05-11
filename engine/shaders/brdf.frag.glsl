#include "common.nocompile.glsl"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec2 outColor;

vec2 IntegrateBRDF(float NdotV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;
    
    float A = 0.0;
    float B = 0.0;
    
    const vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        const vec2 Xi = hammersley(i, SAMPLE_COUNT);
        const vec3 H = importance_sample_ggx(Xi, N, roughness);
        const vec3 L = 2.0 * dot(V, H) * H - V;
        
        const float NdotL = max(L.z, 0.0);
        const float NdotH = max(H.z, 0.0);
        const float VdotH = max(dot(V, H), 0.0);
        
        if(NdotL > 0.0) {
            const float G = geometry_smith(N, V, L, roughness);
            const float G_Vis = (G * VdotH) / (NdotH * NdotV);
            const float Fc = pow(1.0 - VdotH, 5.0);
            
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    
    return vec2(A, B);
}

void main() {
    outColor = IntegrateBRDF(inUV.x, inUV.y);
}

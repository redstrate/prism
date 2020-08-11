const float PI = 3.14159265359;

const vec2 PoissonOffsets[64] = {
    vec2(0.0617981, 0.07294159),
    vec2(0.6470215, 0.7474022),
    vec2(-0.5987766, -0.7512833),
    vec2(-0.693034, 0.6913887),
    vec2(0.6987045, -0.6843052),
    vec2(-0.9402866, 0.04474335),
    vec2(0.8934509, 0.07369385),
    vec2(0.1592735, -0.9686295),
    vec2(-0.05664673, 0.995282),
    vec2(-0.1203411, -0.1301079),
    vec2(0.1741608, -0.1682285),
    vec2(-0.09369049, 0.3196758),
    vec2(0.185363, 0.3213367),
    vec2(-0.1493771, -0.3147511),
    vec2(0.4452095, 0.2580113),
    vec2(-0.1080467, -0.5329178),
    vec2(0.1604507, 0.5460774),
    vec2(-0.4037193, -0.2611179),
    vec2(0.5947998, -0.2146744),
    vec2(0.3276062, 0.9244621),
    vec2(-0.6518704, -0.2503952),
    vec2(-0.3580975, 0.2806469),
    vec2(0.8587891, 0.4838005),
    vec2(-0.1596546, -0.8791054),
    vec2(-0.3096867, 0.5588146),
    vec2(-0.5128918, 0.1448544),
    vec2(0.8581337, -0.424046),
    vec2(0.1562584, -0.5610626),
    vec2(-0.7647934, 0.2709858),
    vec2(-0.3090832, 0.9020988),
    vec2(0.3935608, 0.4609676),
    vec2(0.3929337, -0.5010948),
    vec2(-0.8682281, -0.1990303),
    vec2(-0.01973724, 0.6478714),
    vec2(-0.3897587, -0.4665619),
    vec2(-0.7416366, -0.4377831),
    vec2(-0.5523247, 0.4272514),
    vec2(-0.5325066, 0.8410385),
    vec2(0.3085465, -0.7842533),
    vec2(0.8400612, -0.200119),
    vec2(0.6632416, 0.3067062),
    vec2(-0.4462856, -0.04265022),
    vec2(0.06892014, 0.812484),
    vec2(0.5149567, -0.7502338),
    vec2(0.6464897, -0.4666451),
    vec2(-0.159861, 0.1038342),
    vec2(0.6455986, 0.04419327),
    vec2(-0.7445076, 0.5035095),
    vec2(0.9430245, 0.3139912),
    vec2(0.0349884, -0.7968109),
    vec2(-0.9517487, 0.2963554),
    vec2(-0.7304786, -0.01006928),
    vec2(-0.5862702, -0.5531025),
    vec2(0.3029106, 0.09497032),
    vec2(0.09025345, -0.3503742),
    vec2(0.4356628, -0.0710125),
    vec2(0.4112572, 0.7500054),
    vec2(0.3401214, -0.3047142),
    vec2(-0.2192158, -0.6911137),
    vec2(-0.4676369, 0.6570358),
    vec2(0.6295372, 0.5629555),
    vec2(0.1253822, 0.9892166),
    vec2(-0.1154335, 0.8248222),
    vec2(-0.4230408, -0.7129914)
};

// GGX/Trowbridge-Reitz Normal Distribution
float ggx_distribution(const vec3 N, const vec3 H, const float roughness) {
    const float roughness_squared = roughness * roughness;
    
    const float NdotH = dot(N, H);
    
    const float denominator = (NdotH * NdotH) * (roughness_squared - 1.0) + 1.0;
    
    return roughness_squared / (PI * (denominator * denominator));
}

// Slick Geometry
float geometry_slick_direct(const vec3 N, const vec3 V, const float roughness) {
    const float NdotV = clamp(dot(N, V), 0.0, 1.0);
    const float denominator = NdotV * (1.0 - roughness) + roughness;
    
    return NdotV / denominator;
}

// GGX Smith Geometry, using GGX slick but combining both the view direction and the light direction
float geometry_smith(const vec3 N, const vec3 V, const vec3 L, float roughness) {
    float ggx2  = geometry_slick_direct(N, V, roughness);
    float ggx1  = geometry_slick_direct(N, L, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel Shlick
vec3 fresnel_schlick(const float cos_theta, const vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 fresnel_schlick_roughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// efficient VanDerCorpus calculation from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radical_inverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(const uint i, const uint N) {
    return vec2(float(i) / float(N), radical_inverse(i));
}

vec3 importance_sample_ggx(const vec2 Xi, const vec3 N, const float roughness) {
    const float a = roughness*roughness;
    
    const float phi = 2.0 * PI * Xi.x;
    const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    const float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    // from spherical coordinates to cartesian coordinates - halfway vector
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // from tangent-space H vector to world-space sample vector
    const vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 tangent = normalize(cross(up, N));
    const vec3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

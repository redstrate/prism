struct ComputedSurfaceInfo {
    vec3 N;
    vec3 V;
    vec3 F0, diffuse_color;
    
    float NdotV;
    
    float roughness, metallic;
};

ComputedSurfaceInfo compute_surface(const vec3 diffuse, const vec3 normal, const float metallic, const float roughness) {
    ComputedSurfaceInfo info;
 
    info.N = normalize(normal);
    info.V = normalize(scene.camPos.xyz - in_frag_pos);
    info.NdotV = max(dot(info.N, info.V), 0.0);
    
    info.metallic = metallic;
    info.roughness = max(0.0001, roughness);
    
    info.F0 = 0.16 * (1.0 - info.metallic) + diffuse * info.metallic;
    info.diffuse_color = (1.0 - info.metallic) * diffuse;

    return info;
}

struct SurfaceBRDF {
    vec3 diffuse, specular;
    float NdotL;
};

SurfaceBRDF brdf(const vec3 L, const ComputedSurfaceInfo surface_info) {
    SurfaceBRDF info;
    
    // half-vector
    const vec3 H = normalize(surface_info.V + L);
    const float HdotV = clamp(dot(H, surface_info.V), 0.0, 1.0);

    // fresnel reflectance function
    const vec3 F = fresnel_schlick(surface_info.NdotV, surface_info.F0);    

    // geometry function
    const float D = ggx_distribution(surface_info.N, H, surface_info.roughness);

    // normal distribution function
    const float G = geometry_smith(surface_info.N, surface_info.V, L, surface_info.roughness);

    const vec3 numerator = F * G * D;
    const float denominator = 4.0 * surface_info.NdotV * clamp(dot(surface_info.N, L), 0.0, 1.0);
    
    // cook-torrance specular brdf
    info.specular = numerator / (denominator + 0.001);

    // lambertian diffuse
    info.diffuse = surface_info.diffuse_color * (1.0 / PI);
    
    info.NdotL = clamp(dot(surface_info.N, L), 0.0, 1.0);
    
    return info;
}

struct ComputedLightInformation {
    vec3 direction;
    float radiance;
};

float pcf_sun(const vec4 shadowCoords, const float uvRadius) {
    float sum = 0;
    for(int i = 0; i < 16; i++) {
        const float z = texture(sun_shadow, shadowCoords.xy + PoissonOffsets[i] * uvRadius).r;
        sum += (z < (shadowCoords.z - 0.005)) ? 0.0 : 1.0;
    }
    
    return sum / 16;
}

#ifdef SHADOW_FILTER_PCSS
float search_width(const float uvLightSize, const float receiverDistance) {
    return uvLightSize * (receiverDistance - 0.1f) / receiverDistance;
}

float penumbra_size(const float zReceiver, const float zBlocker) {
    return (zReceiver - zBlocker) / zBlocker;
}

const int numBlockerSearchSamples = 16;

void blocker_distance_sun(const vec3 shadowCoords, const float uvLightSize, inout float avgBlockerDistance, inout int blockers) {
    const float searchWidth = search_width(uvLightSize, shadowCoords.z);

    float blockerSum = 0.0;
    blockers = 0;
    
    for(int i = 0; i < numBlockerSearchSamples; i++) {
        const float z = texture(sun_shadow,
                                shadowCoords.xy + PoissonOffsets[i] * searchWidth).r;
        if(z < shadowCoords.z) {
            blockerSum += z;
            blockers++;
        }
    }
    
    avgBlockerDistance = blockerSum / blockers;
}

float pcss_sun(const vec4 shadowCoords, float light_size_uv) {
    float average_blocker_depth = 0.0;
    int num_blockers = 0;
    blocker_distance_sun(shadowCoords.xyz, light_size_uv, average_blocker_depth, num_blockers);
    if(num_blockers < 1)
        return 1.0;
    
    const float penumbraWidth = penumbra_size(-shadowCoords.z, average_blocker_depth);
    
    const float uvRadius = penumbraWidth * light_size_uv * 0.1 / shadowCoords.z;
    return pcf_sun(shadowCoords, uvRadius);
}
#endif

ComputedLightInformation calculate_sun(Light light) {
    ComputedLightInformation light_info;
    light_info.direction = normalize(light.positionType.xyz - vec3(0));
    
    float shadow = 1.0;
    if(light.shadowsEnable.x == 1.0) {
        const vec4 shadowCoords = fragPosLightSpace / fragPosLightSpace.w;

        if(shadowCoords.z > -1.0 && shadowCoords.z < 1.0) {
#ifdef SHADOW_FILTER_NONE
            shadow = (texture(sun_shadow, shadowCoords.xy).r < shadowCoords.z - 0.005) ? 0.0 : 1.0;
#endif
#ifdef SHADOW_FILTER_PCF
            shadow = pcf_sun(shadowCoords, 0.1);
#endif
#ifdef SHADOW_FILTER_PCSS
            shadow = pcss_sun(shadowCoords, light.shadowsEnable.y);
#endif
        }
    }
    
    light_info.radiance = light.directionPower.w * shadow;
    
    return light_info;
}

float pcf_spot(const vec4 shadowCoords, const int index, const float uvRadius) {
    float sum = 0;
    for(int i = 0; i < 16; i++) {
        const float z = texture(spot_shadow, vec3(shadowCoords.xy + PoissonOffsets[i] * uvRadius, index)).r;
        sum += (z < (shadowCoords.z - 0.001)) ? 0.0 : 1.0;
    }
    
    return sum / 16;
}

#ifdef SHADOW_FILTER_PCSS
void blocker_distance_spot(const vec3 shadowCoords, const int index, const float uvLightSize, inout float avgBlockerDistance, inout int blockers) {
    const float searchWidth = search_width(uvLightSize, shadowCoords.z);
    
    float blockerSum = 0.0;
    blockers = 0;
    
    for(int i = 0; i < numBlockerSearchSamples; i++) {
        const float z = texture(spot_shadow,
                                vec3(shadowCoords.xy + PoissonOffsets[i] * searchWidth, index)).r;
        if(z < shadowCoords.z) {
            blockerSum += z;
            blockers++;
        }
    }
    
    avgBlockerDistance = blockerSum / blockers;
}

float pcss_spot(const vec4 shadowCoords, const int index, float light_size_uv) {
    float average_blocker_depth = 0.0;
    int num_blockers = 0;
    blocker_distance_spot(shadowCoords.xyz, index, light_size_uv, average_blocker_depth, num_blockers);
    if(num_blockers < 1)
        return 1.0;
    
    const float penumbraWidth = penumbra_size(-shadowCoords.z, average_blocker_depth);
    
    const float uvRadius = penumbraWidth * light_size_uv * 0.1 / shadowCoords.z;
    return pcf_spot(shadowCoords, index, uvRadius);
}

#endif

int last_spot_light = 0;

ComputedLightInformation calculate_spot(Light light) {
    ComputedLightInformation light_info;
    light_info.direction = normalize(light.positionType.xyz - in_frag_pos);
    
    float shadow = 1.0;
    if(light.shadowsEnable.x == 1.0) {
        const vec4 shadowCoord = fragPostSpotLightSpace[last_spot_light] / fragPostSpotLightSpace[last_spot_light].w;

#ifdef SHADOW_FILTER_NONE
            shadow = (texture(spot_shadow, vec3(shadowCoord.xy, last_spot_light)).r < shadowCoord.z) ? 0.0 : 1.0;
#endif
#ifdef SHADOW_FILTER_PCF
            shadow = pcf_spot(shadowCoord, last_spot_light, 0.01);
#endif
#ifdef SHADOW_FILTER_PCSS
            shadow = pcss_spot(shadowCoord, last_spot_light, light.shadowsEnable.y);
#endif
        
        last_spot_light++;
    }
    
    const float inner_cutoff = light.colorSize.w + radians(5);
    const float outer_cutoff = light.colorSize.w;
    
    const float theta = dot(light_info.direction, normalize(light.directionPower.xyz));
    const float epsilon = inner_cutoff - outer_cutoff;
    const float intensity = clamp((theta - outer_cutoff) / epsilon, 0.0, 1.0);
    
    light_info.radiance = light.directionPower.w * shadow * intensity;
    
    return light_info;
}

#ifdef POINT_SHADOWS_SUPPORTED

float pcf_point(const vec3 shadowCoords, const int index, const float uvRadius) {
    float sum = 0;
    for(int i = 0; i < 16; i++) {
        const float z = texture(point_shadow, vec4(shadowCoords.xyz + vec3(PoissonOffsets[i].xy, PoissonOffsets[i].x) * uvRadius, index)).r;
        sum += (z < length(shadowCoords) - 0.05) ? 0.0 : 1.0;
    }
    
    return sum / 16;
}

#ifdef SHADOW_FILTER_PCSS
void blocker_distance_point(const vec3 shadowCoords, const int index, const float uvLightSize, inout float avgBlockerDistance, inout int blockers) {
    const float searchWidth = search_width(uvLightSize, length(shadowCoords));

    float blockerSum = 0.0;
    blockers = 0;
    
    for(int i = 0; i < numBlockerSearchSamples; i++) {
        const float z = texture(point_shadow,
                                vec4(shadowCoords + vec3(PoissonOffsets[i], PoissonOffsets[i].x) * searchWidth, index)).r;
        if(z < length(shadowCoords)) {
            blockerSum += z;
            blockers++;
        }
    }
    
    avgBlockerDistance = blockerSum / blockers;
}

float pcss_point(const vec3 shadowCoords, const int index, float light_size_uv) {
    float average_blocker_depth = 0.0;
    int num_blockers = 0;
    blocker_distance_point(shadowCoords.xyz, index, light_size_uv, average_blocker_depth, num_blockers);
    if(num_blockers < 1)
        return 1.0;
    
    const float depth = length(shadowCoords);
    const float penumbraWidth = penumbra_size(-depth, average_blocker_depth);
    
    const float uvRadius = penumbraWidth * light_size_uv;
    return pcf_point(shadowCoords, index, uvRadius);
}
#endif

#endif

int last_point_light = 0;

ComputedLightInformation calculate_point(Light light) {
    ComputedLightInformation light_info;
    light_info.direction = normalize(light.positionType.xyz - in_frag_pos);
    
    const vec3 lightVec = in_frag_pos - light.positionType.xyz;
    
    // Check if fragment is in shadow
    float shadow = 1.0;
#ifdef POINT_SHADOWS_SUPPORTED
    if(light.shadowsEnable.x == 1.0) {
#ifdef SHADOW_FILTER_NONE
        const float sampledDist = texture(point_shadow, vec4(lightVec, last_point_light++)).r;
        const float dist = length(lightVec);
        
        shadow = (dist <= sampledDist + 0.05) ? 1.0 : 0.0;
#endif
#ifdef SHADOW_FILTER_PCF
        shadow = pcf_point(lightVec, last_point_light++, 1.0);
#endif
#ifdef SHADOW_FILTER_PCSS
        shadow = pcss_point(lightVec, last_point_light++, light.shadowsEnable.y);
#endif
    }
#endif
    
    const float distance = length(light.positionType.xyz - in_frag_pos);
    const float attenuation = 1.0 / (distance * distance);
    
    light_info.radiance = attenuation * light.directionPower.w * shadow;
    
    return light_info;
}

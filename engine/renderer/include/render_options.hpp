#pragma once

constexpr int brdf_resolution = 512;

constexpr bool default_enable_aa = true;

enum class ShadowFilter {
    None,
    PCF,
    PCSS
};

#if defined(PLATFORM_TVOS) || defined(PLATFORM_IOS)
constexpr bool default_enable_ibl = false;
constexpr bool default_enable_normal_mapping = false;
constexpr bool default_enable_point_shadows = false;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::PCF;
constexpr int default_shadow_resolution = 1024;
#else
constexpr bool default_enable_ibl = true;
constexpr bool default_enable_normal_mapping = true;
constexpr bool default_enable_point_shadows = true;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::PCSS;
constexpr int default_shadow_resolution = 2048;
#endif

struct RenderOptions {
    bool dynamic_resolution = false;
    double render_scale = 1.0f;
    
    int shadow_resolution = default_shadow_resolution;
    
    bool enable_aa = default_enable_aa;
    bool enable_ibl = default_enable_ibl;
    bool enable_normal_mapping = default_enable_normal_mapping;
    bool enable_normal_shadowing = default_enable_normal_mapping;
    bool enable_point_shadows = default_enable_point_shadows;
    ShadowFilter shadow_filter = default_shadow_filter;
    bool enable_extra_passes = true;
    bool enable_frustum_culling = true;
};

inline RenderOptions render_options;

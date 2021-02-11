#pragma once

constexpr int brdf_resolution = 512;

constexpr bool default_enable_aa = true;

enum class ShadowFilter {
    None,
    PCF,
    PCSS
};

enum class DisplayColorSpace {
    Linear = 0,
    SRGB = 1,
};

enum class TonemapOperator {
    Linear = 0,
    ExposureBased = 1,
    AutoExposure = 2
};

#if defined(PLATFORM_TVOS) || defined(PLATFORM_IOS)
constexpr bool default_enable_ibl = false;
constexpr bool default_enable_normal_mapping = false;
constexpr bool default_enable_point_shadows = false;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::PCF;
constexpr int default_shadow_resolution = 1024;
#endif

#if defined(PLATFORM_WINDOWS)
constexpr bool default_enable_ibl = false;
constexpr bool default_enable_normal_mapping = true;
constexpr bool default_enable_point_shadows = true;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::PCSS;
constexpr int default_shadow_resolution = 2048;
#else
#if defined(PLATFORM_WINDOWS)
constexpr bool default_enable_ibl = false;
constexpr bool default_enable_normal_mapping = false;
constexpr bool default_enable_point_shadows = false;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::None;
constexpr int default_shadow_resolution = 2048;
#else
constexpr bool default_enable_ibl = true;
constexpr bool default_enable_normal_mapping = true;
constexpr bool default_enable_point_shadows = true;
constexpr ShadowFilter default_shadow_filter = ShadowFilter::PCSS;
constexpr int default_shadow_resolution = 2048;
#endif
#endif

struct RenderOptions {
    DisplayColorSpace display_color_space = DisplayColorSpace::SRGB;
    TonemapOperator tonemapping = TonemapOperator::Linear;
    float exposure = 1.0f;
    
    float min_luminance = -8.0f;
    float max_luminance = 3.0f;
    
    bool enable_depth_of_field = false;
    float depth_of_field_strength = 3.0f;
    
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

#include "gfx_metal.hpp"

#include "gfx_metal_buffer.hpp"
#include "gfx_metal_pipeline.hpp"
#include "gfx_commandbuffer.hpp"
#include "gfx_metal_texture.hpp"
#include "gfx_metal_renderpass.hpp"
#include "gfx_metal_framebuffer.hpp"
#include "gfx_metal_sampler.hpp"
#include "file.hpp"
#include "log.hpp"
#include "utility.hpp"
#include "string_utils.hpp"

static inline bool debug_enabled = false;

static inline std::array<GFXCommandBuffer*, 15> command_buffers;
static inline std::array<bool, 15> free_command_buffers;

MTLPixelFormat toPixelFormat(GFXPixelFormat format) {
    switch(format) {
        case GFXPixelFormat::R_32F:
            return MTLPixelFormatR32Float;
        case GFXPixelFormat::R_16F:
            return MTLPixelFormatR16Float;
        case GFXPixelFormat::RGBA_32F:
            return MTLPixelFormatRGBA32Float;
        case GFXPixelFormat::RGBA8_UNORM:
            return MTLPixelFormatRGBA8Unorm;
        case GFXPixelFormat::R8_UNORM:
            return MTLPixelFormatR8Unorm;
        case GFXPixelFormat::R8G8_UNORM:
            return MTLPixelFormatRG8Unorm;
        case GFXPixelFormat::R8G8_SFLOAT:
            return MTLPixelFormatRG16Float;
        case GFXPixelFormat::R8G8B8A8_UNORM:
            return MTLPixelFormatRGBA8Unorm;
        case GFXPixelFormat::R16G16B16A16_SFLOAT:
            return MTLPixelFormatRGBA16Float;
        case GFXPixelFormat::DEPTH_32F:
            return MTLPixelFormatDepth32Float;
    }
}

MTLBlendFactor toBlendFactor(GFXBlendFactor factor) {
    switch(factor) {
        case GFXBlendFactor::One:
            return MTLBlendFactorOne;
        case GFXBlendFactor::Zero:
            return MTLBlendFactorZero;
        case GFXBlendFactor::SrcColor:
            return MTLBlendFactorSourceColor;
        case GFXBlendFactor::DstColor:
            return MTLBlendFactorDestinationColor;
        case GFXBlendFactor::SrcAlpha:
            return MTLBlendFactorSourceAlpha;
        case GFXBlendFactor::DstAlpha:
            return MTLBlendFactorDestinationAlpha;
        case GFXBlendFactor::OneMinusSrcAlpha:
            return MTLBlendFactorOneMinusSourceAlpha;
        case GFXBlendFactor::OneMinusSrcColor:
            return MTLBlendFactorOneMinusSourceColor;
    }
}

MTLSamplerAddressMode toSamplingMode(SamplingMode mode) {
    switch(mode) {
        case SamplingMode::Repeat:
            return MTLSamplerAddressModeRepeat;
        case SamplingMode::ClampToEdge:
            return MTLSamplerAddressModeClampToEdge;
        case SamplingMode::ClampToBorder:
        {
#if defined(PLATFORM_IOS) || defined(PLATFORM_TVOS)
            return MTLSamplerAddressModeRepeat;
#else
            return MTLSamplerAddressModeClampToBorderColor;
#endif
        }
    }
}

#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS)
MTLSamplerBorderColor toBorderColor(GFXBorderColor color) {
    switch(color) {
        case GFXBorderColor::OpaqueWhite:
            return MTLSamplerBorderColorOpaqueWhite;
        case GFXBorderColor::OpaqueBlack:
            return MTLSamplerBorderColorOpaqueBlack;
    }
}
#endif

MTLCompareFunction toCompare(GFXCompareFunction function) {
    switch(function) {
        case GFXCompareFunction::Never:
            return MTLCompareFunctionNever;
        case GFXCompareFunction::Less:
            return MTLCompareFunctionLess;
        case GFXCompareFunction::Equal:
            return MTLCompareFunctionEqual;
        case GFXCompareFunction::LessOrEqual:
            return MTLCompareFunctionLessEqual;
        case GFXCompareFunction::Greater:
            return MTLCompareFunctionGreater;
        case GFXCompareFunction::NotEqual:
            return MTLCompareFunctionNotEqual;
        case GFXCompareFunction::GreaterOrEqual:
            return MTLCompareFunctionGreaterEqual;
        case GFXCompareFunction::Always:
            return MTLCompareFunctionAlways;
    }
}

MTLSamplerMinMagFilter toFilter(GFXFilter filter) {
    switch(filter) {
        case GFXFilter::Nearest:
            return MTLSamplerMinMagFilterNearest;
        case GFXFilter::Linear:
            return MTLSamplerMinMagFilterLinear;
    }
}

MTLWinding toWinding(GFXWindingMode mode) {
    switch(mode) {
        case GFXWindingMode::Clockwise:
            return MTLWindingClockwise;
        case GFXWindingMode::CounterClockwise:
            return MTLWindingCounterClockwise;
    }
}

bool GFXMetal::is_supported() {
    return true;
}

bool GFXMetal::initialize(const GFXCreateInfo& createInfo) {
    debug_enabled = createInfo.api_validation_enabled;
    
    device = MTLCreateSystemDefaultDevice();
    if(device) {
        command_queue = [device newCommandQueue];

        for(int i = 0; i < 15; i++) {
            command_buffers[i] = new GFXCommandBuffer();
            free_command_buffers[i] = true;
        }
        
        return true;
    } else {
        return false;
    }
}

const char* GFXMetal::get_name() {
    return "Metal";
}

bool GFXMetal::supports_feature(const GFXFeature feature) {
    if(feature == GFXFeature::CubemapArray) {
#if defined(PLATFORM_TVOS)
        return false;
#else
        return true;
#endif
    }
    
    return false;
}

void GFXMetal::initialize_view(void* native_handle, const int identifier, const uint32_t, const uint32_t) {
    NativeMTLView* native = new NativeMTLView();
    native->identifier = identifier;

    native->layer = (CAMetalLayer*)native_handle;
    native->layer.device = device;
    
    native->layer.allowsNextDrawableTimeout = true;

    nativeViews.push_back(native);
}

void GFXMetal::remove_view(const int identifier) {
    utility::erase_if(nativeViews, [identifier](NativeMTLView* view) {
        return view->identifier == identifier;
    });
}

GFXBuffer* GFXMetal::create_buffer(void* data, const GFXSize size, const bool dynamicData, const GFXBufferUsage) {
    GFXMetalBuffer* buffer = new GFXMetalBuffer();
    buffer->dynamicData = dynamicData;

    if(buffer->dynamicData) {
        for(int i = 0; i < 3; i++) {
            if(data == nullptr) {
                buffer->handles[i] = [device newBufferWithLength:(NSUInteger)size options:MTLResourceStorageModeShared];
            } else {
                buffer->handles[i] = [device newBufferWithBytes:data
                                                     length:(NSUInteger)size
                                                    options:MTLResourceStorageModeShared];
            }
        }
    } else {
        if(data == nullptr) {
            buffer->handles[0] = [device newBufferWithLength:(NSUInteger)size options:MTLResourceStorageModeShared];
        } else {
            buffer->handles[0] = [device newBufferWithBytes:data
                                                 length:(NSUInteger)size
                                                options:MTLResourceStorageModeShared];
        }
    }

    return buffer;
}

int currentFrameIndex = 0;

void GFXMetal::copy_buffer(GFXBuffer* buffer, void* data, const GFXSize offset, const GFXSize size) {
    GFXMetalBuffer* metalBuffer = (GFXMetalBuffer*)buffer;

    const unsigned char * src = reinterpret_cast<const unsigned char*>(data);
    unsigned char * dest = reinterpret_cast<unsigned char *>(metalBuffer->get(currentFrameIndex).contents);
    memcpy(dest + offset, src, size);

    //[metalBuffer->handle didModifyRange:NSMakeRange(offset, size)];
}

void* GFXMetal::get_buffer_contents(GFXBuffer* buffer) {
    GFXMetalBuffer* metalBuffer = (GFXMetalBuffer*)buffer;

    return reinterpret_cast<unsigned char *>(metalBuffer->get(currentFrameIndex).contents);
}

GFXTexture* GFXMetal::create_texture(const GFXTextureCreateInfo& info) {
    GFXMetalTexture* texture = new GFXMetalTexture();

    MTLTextureDescriptor* textureDescriptor = [[MTLTextureDescriptor alloc] init];

    MTLPixelFormat mtlFormat = toPixelFormat(info.format);

    switch(info.type) {
        case GFXTextureType::Single2D:
            textureDescriptor.textureType = MTLTextureType2D;
            break;
        case GFXTextureType::Array2D:
            textureDescriptor.textureType = MTLTextureType2DArray;
            break;
        case GFXTextureType::Cubemap:
        {
            textureDescriptor.textureType = MTLTextureTypeCube;
            texture->is_cubemap = true;
        }
            break;
        case GFXTextureType::CubemapArray:
        {
            textureDescriptor.textureType = MTLTextureTypeCubeArray;
            texture->is_cubemap = true;
        }
            break;
    }
    
    if((info.usage & GFXTextureUsage::Attachment) == GFXTextureUsage::Attachment) {
        textureDescriptor.storageMode = MTLStorageModePrivate;
        textureDescriptor.usage |= MTLTextureUsageRenderTarget;
    } else {
#if defined(PLATFORM_IOS) || defined(PLATFORM_TVOS)
        textureDescriptor.storageMode = MTLStorageModeShared;
#else
        textureDescriptor.storageMode = MTLStorageModeManaged;
#endif
    }
    
    if((info.usage & GFXTextureUsage::Sampled) == GFXTextureUsage::Sampled) {
       textureDescriptor.usage |= MTLTextureUsageShaderRead;
    }
    
    if((info.usage & GFXTextureUsage::ShaderWrite) == GFXTextureUsage::ShaderWrite) {
        textureDescriptor.usage |= MTLTextureUsageShaderWrite;
    }
    
    textureDescriptor.pixelFormat = mtlFormat;
    textureDescriptor.width = info.width;
    textureDescriptor.height = info.height;
    textureDescriptor.arrayLength = info.array_length;
    
    texture->array_length = info.array_length;
    
    textureDescriptor.mipmapLevelCount = info.mip_count;

    texture->format = mtlFormat;

    texture->handle = [device newTextureWithDescriptor:textureDescriptor];

    texture->width = info.width;
    texture->height = info.height;

    MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
    samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
    samplerDescriptor.sAddressMode = toSamplingMode(info.samplingMode);
    samplerDescriptor.tAddressMode = toSamplingMode(info.samplingMode);
    samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
    samplerDescriptor.maxAnisotropy = 16;

#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS)
    samplerDescriptor.borderColor = toBorderColor(info.border_color);
#endif
    
    if(info.compare_enabled)
        samplerDescriptor.compareFunction = toCompare(info.compare_function);

    texture->sampler = [device newSamplerStateWithDescriptor:samplerDescriptor];

    return texture;
}

void GFXMetal::copy_texture(GFXTexture* texture, void* data, const GFXSize) {
    GFXMetalTexture* metalTexture = (GFXMetalTexture*)texture;

    MTLRegion region = {
        { 0, 0, 0 },
        {
            static_cast<NSUInteger>(texture->width),
            static_cast<NSUInteger>(texture->height),
            1
        }
    };

    NSUInteger byteSize = 1;
    if(metalTexture->format == MTLPixelFormatRGBA8Unorm)
        byteSize = 4;
    else if(metalTexture->format == MTLPixelFormatRG8Unorm)
        byteSize = 2;

    [metalTexture->handle replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:(NSUInteger)texture->width * byteSize];
}

void GFXMetal::copy_texture(GFXTexture* from, GFXTexture* to) {
    GFXMetalTexture* metalFromTexture = (GFXMetalTexture*)from;
    GFXMetalTexture* metalToTexture = (GFXMetalTexture*)to;

    id<MTLCommandBuffer> commandBuffer = [command_queue commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    [commandEncoder copyFromTexture:metalFromTexture->handle toTexture:metalToTexture->handle];
    [commandEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

void GFXMetal::copy_texture(GFXTexture* from, GFXBuffer* to) {
    GFXMetalTexture* metalFromTexture = (GFXMetalTexture*)from;
    GFXMetalBuffer* metalToBuffer = (GFXMetalBuffer*)to;

    MTLOrigin origin;
    origin.x = 0;
    origin.y = 0;
    origin.z = 0;

    MTLSize size;
    size.width = from->width;
    size.height = from->height;
    size.depth = 1;
    
    NSUInteger byteSize = 1;
    if(metalFromTexture->format == MTLPixelFormatRGBA8Unorm)
        byteSize = 4;
    
    id<MTLCommandBuffer> commandBuffer = [command_queue commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    [commandEncoder copyFromTexture:metalFromTexture->handle
                    sourceSlice:0
                    sourceLevel:0
                    sourceOrigin:origin
                    sourceSize:size
                    toBuffer:metalToBuffer->get(currentFrameIndex)
                    destinationOffset:0
                    destinationBytesPerRow:(NSUInteger)metalFromTexture->width * byteSize
                    destinationBytesPerImage:0];
    [commandEncoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

GFXSampler* GFXMetal::create_sampler(const GFXSamplerCreateInfo& info) {
    GFXMetalSampler* sampler = new GFXMetalSampler();
    
    MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
    samplerDescriptor.minFilter = toFilter(info.min_filter);
    samplerDescriptor.magFilter = toFilter(info.mag_filter);
    samplerDescriptor.sAddressMode = toSamplingMode(info.samplingMode);
    samplerDescriptor.tAddressMode = toSamplingMode(info.samplingMode);
    samplerDescriptor.mipFilter = MTLSamplerMipFilterLinear;
    samplerDescriptor.maxAnisotropy = 16;
    
#if !defined(PLATFORM_IOS) && !defined(PLATFORM_TVOS)
    samplerDescriptor.borderColor = toBorderColor(info.border_color);
#endif
    
    if(info.compare_enabled)
        samplerDescriptor.compareFunction = toCompare(info.compare_function);
    
    sampler->handle = [device newSamplerStateWithDescriptor:samplerDescriptor];
    
    return sampler;
}

GFXFramebuffer* GFXMetal::create_framebuffer(const GFXFramebufferCreateInfo& info) {
    GFXMetalFramebuffer* framebuffer = new GFXMetalFramebuffer();

    for(auto& attachment : info.attachments)
        framebuffer->attachments.push_back((GFXMetalTexture*)attachment);

    return framebuffer;
}

GFXRenderPass* GFXMetal::create_render_pass(const GFXRenderPassCreateInfo& info) {
    GFXMetalRenderPass* renderPass = new GFXMetalRenderPass();

    for(const auto& attachment : info.attachments)
        renderPass->attachments.push_back(toPixelFormat(attachment));

    return renderPass;
}

MTLFunctionConstantValues* get_constant_values(GFXShaderConstants constants) {
    MTLFunctionConstantValues* constantValues = [MTLFunctionConstantValues new];
    
    for(auto& constant : constants) {
        switch(constant.type) {
            case GFXShaderConstant::Type::Integer:
                [constantValues setConstantValue:&constant.value type:MTLDataTypeInt atIndex:constant.index];
                break;
        }
    }
    
    return constantValues;
}

GFXPipeline* GFXMetal::create_graphics_pipeline(const GFXGraphicsPipelineCreateInfo& info) {
    GFXMetalPipeline* pipeline = new GFXMetalPipeline();

    NSError* error = nil;
    
    // vertex
    id<MTLLibrary> vertexLibrary;
    {
        std::string vertex_src;
        if(info.shaders.vertex_path.empty()) {
            vertex_src = info.shaders.vertex_src.as_string();
        } else {
            const auto vertex_path = utility::format("{}.msl", info.shaders.vertex_path);

            auto file = file::open(file::internal_domain / vertex_path);
            if(file != std::nullopt) {
                vertex_src = file->read_as_string();
            } else {
                console::error(System::GFX, "Failed to load vertex shader from {}!", vertex_path);
            }
        }
        
        vertexLibrary = [device newLibraryWithSource:[NSString stringWithFormat:@"%s", vertex_src.c_str()] options:nil error:&error];
        if(!vertexLibrary)
            NSLog(@"%@", error.debugDescription);
    }
        
    // fragment
    id<MTLLibrary> fragmentLibrary;
    {
        std::string fragment_src;
        if(info.shaders.fragment_path.empty()) {
            fragment_src = info.shaders.fragment_src.as_string();
        } else {
            const auto fragment_path = utility::format("{}.msl", info.shaders.fragment_path);
            
            auto file = file::open(file::internal_domain / fragment_path);
            if(file != std::nullopt) {
                fragment_src = file->read_as_string();
            } else {
                console::error(System::GFX, "Failed to load fragment shader from {}!", fragment_path);
            }
        }
        
        fragmentLibrary = [device newLibraryWithSource:[NSString stringWithFormat:@"%s", fragment_src.c_str()] options:nil error:&error];
        if(!fragmentLibrary)
            NSLog(@"%@", error.debugDescription);
    }
    
    auto vertex_constants = get_constant_values(info.shaders.vertex_constants);
    
    id<MTLFunction> vertexFunc = [vertexLibrary newFunctionWithName:@"main0" constantValues:vertex_constants error:nil];
    
    auto fragment_constants = get_constant_values(info.shaders.fragment_constants);
    
    id<MTLFunction> fragmentFunc = [fragmentLibrary newFunctionWithName:@"main0" constantValues:fragment_constants error:nil];

    if(debug_enabled) {
        vertexFunc.label = [NSString stringWithFormat:@"%s", info.shaders.vertex_path.data()];
        fragmentFunc.label = [NSString stringWithFormat:@"%s", info.shaders.fragment_path.data()];
    }
    
    MTLVertexDescriptor* descriptor = [MTLVertexDescriptor new];

    for(auto input : info.vertex_input.inputs) {
        descriptor.layouts[input.location].stride = (NSUInteger)input.stride;
        descriptor.layouts[input.location].stepFunction = MTLVertexStepFunctionPerVertex;

        GFXMetalPipeline::VertexStride vs;
        vs.location = input.location;
        vs.stride = input.stride;

        pipeline->vertexStrides.push_back(vs);
    }

    for(auto attribute : info.vertex_input.attributes) {
        NSUInteger format = MTLVertexFormatFloat3;
        switch(attribute.format) {
            case GFXVertexFormat::FLOAT2:
                format = MTLVertexFormatFloat2;
                break;
            case GFXVertexFormat::FLOAT3:
                format = MTLVertexFormatFloat3;
                break;
            case GFXVertexFormat::FLOAT4:
                format = MTLVertexFormatFloat4;
                break;
            case GFXVertexFormat::INT:
                format = MTLVertexFormatInt;
                break;
            case GFXVertexFormat::UNORM4:
                format = MTLVertexFormatUChar4Normalized;
                break;
            case GFXVertexFormat::INT4:
                format = MTLVertexFormatInt4;
                break;
        }

        descriptor.attributes[attribute.location].format = (MTLVertexFormat)format;
        descriptor.attributes[attribute.location].bufferIndex = (NSUInteger)attribute.binding;
        descriptor.attributes[attribute.location].offset =  (NSUInteger)attribute.offset;
    }

    MTLRenderPipelineDescriptor *pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = vertexFunc;
    pipelineDescriptor.fragmentFunction = fragmentFunc;

    if(info.render_pass != nullptr) {
        GFXMetalRenderPass* metalRenderPass = (GFXMetalRenderPass*)info.render_pass;

        unsigned int i = 0;
        for(const auto& attachment : metalRenderPass->attachments) {
            if(attachment != MTLPixelFormatDepth32Float) {
                pipelineDescriptor.colorAttachments[i].pixelFormat = attachment;
                pipelineDescriptor.colorAttachments[i].blendingEnabled = info.blending.enable_blending;

                pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor = toBlendFactor(info.blending.src_rgb);
                pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor = toBlendFactor(info.blending.dst_rgb);

                pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor = toBlendFactor(info.blending.src_alpha);
                pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor = toBlendFactor(info.blending.dst_alpha);

                i++;
            } else {
                pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
            }
        }
    } else {
        pipelineDescriptor.colorAttachments[0].pixelFormat = [nativeViews[0]->layer pixelFormat];
        pipelineDescriptor.colorAttachments[0].blendingEnabled = info.blending.enable_blending;

        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = toBlendFactor(info.blending.src_rgb);
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = toBlendFactor(info.blending.dst_rgb);

        pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = toBlendFactor(info.blending.src_alpha);
        pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = toBlendFactor(info.blending.dst_alpha);
    }

    pipelineDescriptor.vertexDescriptor = descriptor;

    if(debug_enabled) {
        pipelineDescriptor.label = [NSString stringWithFormat:@"%s", info.label.data()];
        pipeline->label = info.label;
    }
    
    pipeline->handle = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if(!pipeline->handle)
        NSLog(@"%@", error.debugDescription);

    switch(info.rasterization.primitive_type) {
        case GFXPrimitiveType::Triangle:
            pipeline->primitiveType = MTLPrimitiveTypeTriangle;
            break;
        case GFXPrimitiveType::TriangleStrip:
            pipeline->primitiveType = MTLPrimitiveTypeTriangleStrip;
            break;
    }

    for(auto& binding : info.shader_input.bindings) {
        if(binding.type == GFXBindingType::PushConstant)
            pipeline->pushConstantIndex = binding.binding;
    }

    pipeline->winding_mode = info.rasterization.winding_mode;
    
    MTLDepthStencilDescriptor* depthStencil = [MTLDepthStencilDescriptor new];

    if(info.depth.depth_mode != GFXDepthMode::None) {
        switch(info.depth.depth_mode) {
            case GFXDepthMode::Less:
                depthStencil.depthCompareFunction = MTLCompareFunctionLess;
                break;
            case GFXDepthMode::LessOrEqual:
                depthStencil.depthCompareFunction = MTLCompareFunctionLessEqual;
                break;
            case GFXDepthMode::Greater:
                depthStencil.depthCompareFunction = MTLCompareFunctionGreater;
                break;
        }
        depthStencil.depthWriteEnabled = true;
    }

    pipeline->depthStencil = [device newDepthStencilStateWithDescriptor:depthStencil];

    switch(info.rasterization.culling_mode) {
        case GFXCullingMode::Frontface:
            pipeline->cullMode = MTLCullModeFront;
            break;
        case GFXCullingMode::Backface:
            pipeline->cullMode = MTLCullModeBack;
            break;
        case GFXCullingMode::None:
            pipeline->cullMode = MTLCullModeNone;
            break;
    }

    if(info.rasterization.polygon_type == GFXPolygonType::Line)
        pipeline->renderWire = true;
    
    [vertexFunc release];
    [fragmentFunc release];
    
    [vertexLibrary release];
    [fragmentLibrary release];
    
    [vertex_constants release];
    [fragment_constants release];
    
    return pipeline;
}

GFXPipeline* GFXMetal::create_compute_pipeline(const GFXComputePipelineCreateInfo& info) {
    GFXMetalPipeline* pipeline = new GFXMetalPipeline();
    
    NSError* error = nil;
    
    // vertex
    id<MTLLibrary> computeLibrary;
    {
        std::string compute_src;
        if(info.shaders.compute_path.empty()) {
            compute_src = info.shaders.compute_src.as_string();
        } else {
            const auto compute_path = utility::format("{}.msl", info.shaders.compute_path);
            
            auto file = file::open(file::internal_domain / compute_path);
            if(file != std::nullopt) {
                compute_src = file->read_as_string();
            } else {
                console::error(System::GFX, "Failed to load compute shader from {}!", compute_path);
            }
        }
        
        computeLibrary = [device newLibraryWithSource:[NSString stringWithFormat:@"%s", compute_src.c_str()] options:nil error:&error];
        if(!computeLibrary)
            NSLog(@"%@", error.debugDescription);
    }
    
    id<MTLFunction> computeFunc = [computeLibrary newFunctionWithName:@"main0"];

    MTLComputePipelineDescriptor* pipelineDescriptor = [MTLComputePipelineDescriptor new];
    pipelineDescriptor.computeFunction = computeFunc;
    
    pipeline->threadGroupSize = MTLSizeMake(info.workgroup_size_x, info.workgroup_size_y, info.workgroup_size_z);
    
    if(debug_enabled) {
        pipelineDescriptor.label = [NSString stringWithFormat:@"%s", info.label.data()];
        pipeline->label = info.label;
    }
    
    pipeline->compute_handle = [device newComputePipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if(!pipeline->handle)
        NSLog(@"%@", error.debugDescription);
    
    for(auto& binding : info.shader_input.bindings) {
        if(binding.type == GFXBindingType::PushConstant)
            pipeline->pushConstantIndex = binding.binding;
    }
    
    [computeLibrary release];

    return pipeline;
}

GFXCommandBuffer* GFXMetal::acquire_command_buffer() {
    GFXCommandBuffer* cmdbuf = nullptr;
    while(cmdbuf == nullptr) {
        for(const auto [i, buffer_status] : utility::enumerate(free_command_buffers)) {
            if(buffer_status) {
                GFXCommandBuffer* buffer = command_buffers[i];
                
                free_command_buffers[i] = false;
                
                buffer->commands.clear();
                
                return buffer;
            }
        }
    }

    return cmdbuf;
}

void GFXMetal::submit(GFXCommandBuffer* command_buffer, const int window) {
    @autoreleasepool {
        NativeMTLView* native = getNativeView(window);
        id<CAMetalDrawable> drawable = nil;
        if(native != nullptr)
            drawable = [native->layer nextDrawable];
        
        id<MTLCommandBuffer> commandBuffer = [command_queue commandBuffer];

        id <MTLRenderCommandEncoder> renderEncoder = nil;
        id <MTLComputeCommandEncoder> computeEncoder = nil;
        id <MTLBlitCommandEncoder> blitEncoder = nil;
        
        GFXMetalRenderPass* currentRenderPass = nullptr;
        GFXMetalFramebuffer* currentFramebuffer = nullptr;
        GFXMetalPipeline* currentPipeline = nullptr;
        GFXMetalBuffer* currentIndexBuffer = nullptr;
        IndexType currentIndextype = IndexType::UINT32;
        MTLViewport currentViewport = MTLViewport();
        MTLClearColor currentClearColor;
        
        enum class CurrentEncoder {
            None,
            Render,
            Compute,
            Blit
        } current_encoder = CurrentEncoder::None;

        const auto needEncoder = [&](CurrentEncoder encoder, bool needs_reset = false) {
            if(encoder != current_encoder || needs_reset) {
                if(renderEncoder != nil)
                    [renderEncoder endEncoding];
                
                if(computeEncoder != nil)
                    [computeEncoder endEncoding];
                
                if(blitEncoder != nil)
                    [blitEncoder endEncoding];
                
                renderEncoder = nil;
                computeEncoder = nil;
                blitEncoder = nil;
            }
            
            if(current_encoder == encoder && !needs_reset)
                return;
            
            switch(encoder) {
                case CurrentEncoder::None:
                    break;
                case CurrentEncoder::Render:
                {
                    MTLRenderPassDescriptor* descriptor = [MTLRenderPassDescriptor new];

                    if(currentRenderPass != nullptr && currentFramebuffer != nullptr) {
                        unsigned int i = 0;
                        for(const auto& attachment : currentFramebuffer->attachments) {
                            if(attachment->format == MTLPixelFormatDepth32Float) {
                                descriptor.depthAttachment.texture = attachment->handle;
                                descriptor.depthAttachment.loadAction = MTLLoadActionClear;
                                descriptor.depthAttachment.storeAction = MTLStoreActionStore;
                            } else {
                                descriptor.colorAttachments[i].texture = attachment->handle;
                                descriptor.colorAttachments[i].loadAction = MTLLoadActionClear;
                                descriptor.colorAttachments[i].storeAction = MTLStoreActionStore;
                                descriptor.colorAttachments[i].clearColor = currentClearColor;
                                
                                i++;
                            }
                        }
                        
                        renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
                    } else {
                        descriptor.colorAttachments[0].texture = drawable.texture;
                        descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
                        descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0,0.0,0.0,1.0);
                        
                        renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
                    }
                 
                    if(currentViewport.width != 0.0f && currentViewport.height != 0.0f)
                        [renderEncoder setViewport:currentViewport];
                    
                    [descriptor release];
                }
                    break;
                case CurrentEncoder::Compute:
                {
                    computeEncoder = [commandBuffer computeCommandEncoder];
                }
                    break;
                case CurrentEncoder::Blit:
                {
                    blitEncoder = [commandBuffer blitCommandEncoder];
                }
                    break;
            }
            
            current_encoder = encoder;
        };

        for(auto command : command_buffer->commands) {
            switch(command.type) {
                case GFXCommandType::Invalid:
                    break;
                case GFXCommandType::SetRenderPass:
                {
                    currentClearColor = MTLClearColorMake(command.data.set_render_pass.clear_color.r,
                                                          command.data.set_render_pass.clear_color.g,
                                                          command.data.set_render_pass.clear_color.b,
                                                          command.data.set_render_pass.clear_color.a );
                    
                    currentFramebuffer = (GFXMetalFramebuffer*)command.data.set_render_pass.framebuffer;
                    currentRenderPass = (GFXMetalRenderPass*)command.data.set_render_pass.render_pass;
                    
                    currentViewport = MTLViewport();
                    
                    needEncoder(CurrentEncoder::Render, true);
                }
                    break;
                case GFXCommandType::SetGraphicsPipeline:
                {
                    needEncoder(CurrentEncoder::Render);
                    
                    [renderEncoder setRenderPipelineState:((GFXMetalPipeline*)command.data.set_graphics_pipeline.pipeline)->handle];

                    currentPipeline = (GFXMetalPipeline*)command.data.set_graphics_pipeline.pipeline;

                    [renderEncoder setDepthStencilState:currentPipeline->depthStencil];

                    [renderEncoder setCullMode:((GFXMetalPipeline*)command.data.set_graphics_pipeline.pipeline)->cullMode];
                    [renderEncoder setFrontFacingWinding:toWinding(((GFXMetalPipeline*)command.data.set_graphics_pipeline.pipeline)->winding_mode)];

                    if(currentPipeline->renderWire)
                        [renderEncoder setTriangleFillMode:MTLTriangleFillModeLines];
                    else
                        [renderEncoder setTriangleFillMode:MTLTriangleFillModeFill];
                }
                    break;
                case GFXCommandType::SetComputePipeline:
                {
                    needEncoder(CurrentEncoder::Compute);
                    
                    currentPipeline = (GFXMetalPipeline*)command.data.set_compute_pipeline.pipeline;
                    
                    [computeEncoder setComputePipelineState:((GFXMetalPipeline*)command.data.set_compute_pipeline.pipeline)->compute_handle];

                }
                    break;
                case  GFXCommandType::SetVertexBuffer:
                {
                    needEncoder(CurrentEncoder::Render);

                    [renderEncoder setVertexBuffer:((GFXMetalBuffer*)command.data.set_vertex_buffer.buffer)->get(currentFrameIndex) offset:(NSUInteger)command.data.set_vertex_buffer.offset atIndex:(NSUInteger)command.data.set_vertex_buffer.index ];
                }
                    break;
                case GFXCommandType::SetIndexBuffer:
                {
                    currentIndexBuffer = (GFXMetalBuffer*)command.data.set_index_buffer.buffer;
                    currentIndextype = command.data.set_index_buffer.index_type;
                }
                    break;
                case GFXCommandType::SetPushConstant:
                {
                    if(currentPipeline == nullptr)
                        continue;
                    
                    if(current_encoder == CurrentEncoder::Render) {
                        [renderEncoder setVertexBytes:command.data.set_push_constant.bytes.data() length:(NSUInteger)command.data.set_push_constant.size atIndex:(NSUInteger)currentPipeline->pushConstantIndex];
                        [renderEncoder setFragmentBytes:command.data.set_push_constant.bytes.data() length:(NSUInteger)command.data.set_push_constant.size atIndex:(NSUInteger)currentPipeline->pushConstantIndex];
                    } else if(current_encoder == CurrentEncoder::Compute) {
                        [computeEncoder setBytes:command.data.set_push_constant.bytes.data() length:(NSUInteger)command.data.set_push_constant.size atIndex:(NSUInteger)currentPipeline->pushConstantIndex];
                    }
                }
                    break;
                case GFXCommandType::BindShaderBuffer:
                {
                    if(current_encoder == CurrentEncoder::Render) {
                        [renderEncoder setVertexBuffer:((GFXMetalBuffer*)command.data.bind_shader_buffer.buffer)->get(currentFrameIndex) offset:(NSUInteger)command.data.bind_shader_buffer.offset atIndex:(NSUInteger)command.data.bind_shader_buffer.index ];
                        
                        [renderEncoder setFragmentBuffer:((GFXMetalBuffer*)command.data.bind_shader_buffer.buffer)->get(currentFrameIndex) offset:(NSUInteger)command.data.bind_shader_buffer.offset atIndex:(NSUInteger)command.data.bind_shader_buffer.index ];
                    } else if(current_encoder == CurrentEncoder::Compute) {
                        [computeEncoder setBuffer:((GFXMetalBuffer*)command.data.bind_shader_buffer.buffer)->get(currentFrameIndex) offset:(NSUInteger)command.data.bind_shader_buffer.offset atIndex:(NSUInteger)command.data.bind_shader_buffer.index ];
                    }
                }
                    break;
                case GFXCommandType::BindTexture:
                {
                    if(current_encoder == CurrentEncoder::Render) {
                        if(command.data.bind_texture.texture != nullptr) {
                            [renderEncoder setVertexSamplerState:((GFXMetalTexture*)command.data.bind_texture.texture)->sampler atIndex:(NSUInteger)command.data.bind_texture.index];
                            
                            [renderEncoder setVertexTexture:((GFXMetalTexture*)command.data.bind_texture.texture)->handle atIndex:(NSUInteger)command.data.bind_texture.index];
                            
                            [renderEncoder setFragmentSamplerState:((GFXMetalTexture*)command.data.bind_texture.texture)->sampler atIndex:(NSUInteger)command.data.bind_texture.index];

                            [renderEncoder setFragmentTexture:((GFXMetalTexture*)command.data.bind_texture.texture)->handle atIndex:(NSUInteger)command.data.bind_texture.index];
                        } else {
                            [renderEncoder setVertexTexture:nil atIndex:(NSUInteger)command.data.bind_texture.index];
                            
                            [renderEncoder setFragmentTexture:nil atIndex:(NSUInteger)command.data.bind_texture.index];
                        }
                    } else if(current_encoder == CurrentEncoder::Compute) {
                        [computeEncoder setTexture:((GFXMetalTexture*)command.data.bind_texture.texture)->handle atIndex:(NSUInteger)command.data.bind_texture.index];
                    }
                }
                    break;
                case GFXCommandType::BindSampler:
                {
                    needEncoder(CurrentEncoder::Render);
                    
                    if(command.data.bind_sampler.sampler != nullptr) {
                        [renderEncoder setFragmentSamplerState:((GFXMetalSampler*)command.data.bind_sampler.sampler)->handle atIndex:(NSUInteger)command.data.bind_sampler.index];
                    } else {
                        [renderEncoder setFragmentSamplerState:nil atIndex:(NSUInteger)command.data.bind_sampler.index];
                    }
                }
                    break;
                case GFXCommandType::Draw:
                {
                    needEncoder(CurrentEncoder::Render);

                    if(currentPipeline == nullptr)
                        continue;
                    
                    [renderEncoder drawPrimitives:currentPipeline->primitiveType vertexStart:(NSUInteger)command.data.draw.vertex_offset vertexCount:(NSUInteger)command.data.draw.vertex_count instanceCount:(NSUInteger)command.data.draw.instance_count
                        baseInstance:(NSUInteger)command.data.draw.base_instance];
                }
                    break;
                case GFXCommandType::DrawIndexed:
                {
                    needEncoder(CurrentEncoder::Render);

                    if(currentIndexBuffer == nullptr)
                        continue;
                    
                    if(currentPipeline == nullptr)
                        continue;
                    
                    MTLIndexType indexType;
                    int indexSize;
                    switch(currentIndextype) {
                        case IndexType::UINT16:
                        {
                            indexType = MTLIndexTypeUInt16;
                            indexSize = sizeof(uint16_t);
                        }
                            break;
                        case IndexType::UINT32:
                        {
                            indexType = MTLIndexTypeUInt32;
                            indexSize = sizeof(uint32_t);
                        }
                            break;
                    }

                    for(auto& stride : currentPipeline->vertexStrides)
                        [renderEncoder setVertexBufferOffset:(NSUInteger)command.data.draw_indexed.vertex_offset * stride.stride atIndex:stride.location];

                    [renderEncoder
                    drawIndexedPrimitives:currentPipeline->primitiveType
                    indexCount:(NSUInteger)command.data.draw_indexed.index_count
                    indexType:indexType
                    indexBuffer:currentIndexBuffer->get(currentFrameIndex)
                    indexBufferOffset:(NSUInteger)command.data.draw_indexed.first_index * indexSize];
                }
                    break;
                case GFXCommandType::MemoryBarrier:
                {
                    needEncoder(CurrentEncoder::Render);

                    #ifdef PLATFORM_MACOS
                    [renderEncoder memoryBarrierWithScope:MTLBarrierScopeTextures afterStages:MTLRenderStageFragment beforeStages:MTLRenderStageFragment];
                    #endif
                }
                    break;
                case GFXCommandType::CopyTexture:
                {
                    needEncoder(CurrentEncoder::Blit);

                    GFXMetalTexture* metalFromTexture = (GFXMetalTexture*)command.data.copy_texture.src;
                    GFXMetalTexture* metalToTexture = (GFXMetalTexture*)command.data.copy_texture.dst;
                    if(metalFromTexture != nullptr && metalToTexture != nullptr) {
                        const int slice_offset = command.data.copy_texture.to_slice + command.data.copy_texture.to_layer * 6;

                        [blitEncoder
                         copyFromTexture:metalFromTexture->handle
                         sourceSlice:0
                         sourceLevel:0
                         sourceOrigin:MTLOriginMake(0, 0, 0)
                         sourceSize:MTLSizeMake(command.data.copy_texture.width, command.data.copy_texture.height, 1)
                         toTexture:metalToTexture->handle
                         destinationSlice: slice_offset
                         destinationLevel:command.data.copy_texture.to_level
                         destinationOrigin: MTLOriginMake(0, 0, 0)];
                    }
                }
                    break;
                case GFXCommandType::SetViewport:
                {
                    MTLViewport viewport;
                    viewport.originX = command.data.set_viewport.viewport.x;
                    viewport.originY = command.data.set_viewport.viewport.y;
                    viewport.width = command.data.set_viewport.viewport.width;
                    viewport.height = command.data.set_viewport.viewport.height;
                    viewport.znear = command.data.set_viewport.viewport.min_depth;
                    viewport.zfar = command.data.set_viewport.viewport.max_depth;
                    
                    if(renderEncoder != nil)
                        [renderEncoder setViewport:viewport];
                    
                    currentViewport = viewport;
                }
                    break;
                case GFXCommandType::SetScissor:
                {
                    needEncoder(CurrentEncoder::Render);

                    MTLScissorRect rect;
                    rect.x = (NSUInteger)command.data.set_scissor.rect.offset.x;
                    rect.y = (NSUInteger)command.data.set_scissor.rect.offset.y;
                    rect.width = (NSUInteger)command.data.set_scissor.rect.extent.width;
                    rect.height = (NSUInteger)command.data.set_scissor.rect.extent.height;

                    [renderEncoder setScissorRect:rect];
                }
                    break;
                case GFXCommandType::GenerateMipmaps: {
                    needEncoder(CurrentEncoder::Blit);

                    GFXMetalTexture* metalTexture = (GFXMetalTexture*)command.data.generate_mipmaps.texture;
                                        
                    [blitEncoder generateMipmapsForTexture: metalTexture->handle];
                }
                    break;
                case GFXCommandType::SetDepthBias: {
                    needEncoder(CurrentEncoder::Render);
                    
                    [renderEncoder setDepthBias:command.data.set_depth_bias.constant
                                    slopeScale:command.data.set_depth_bias.slope_factor
                                          clamp:command.data.set_depth_bias.clamp];
                }
                    break;
                case GFXCommandType::PushGroup: {
                    [commandBuffer pushDebugGroup:[NSString stringWithUTF8String:command.data.push_group.name.data()]];
                }
                    break;
                case GFXCommandType::PopGroup: {
                    [commandBuffer popDebugGroup];
                }
                    break;
                case GFXCommandType::InsertLabel: {
                    switch(current_encoder) {
                        case CurrentEncoder::Render:
                            [renderEncoder insertDebugSignpost:[NSString stringWithUTF8String:command.data.insert_label.name.data()]];
                            break;
                        case CurrentEncoder::Blit:
                            [blitEncoder insertDebugSignpost:[NSString stringWithUTF8String:command.data.insert_label.name.data()]];
                            break;
                        default:
                            break;
                    }
                }
                    break;
                case GFXCommandType::Dispatch: {
                    needEncoder(CurrentEncoder::Compute);

                    [computeEncoder dispatchThreadgroups:MTLSizeMake(command.data.dispatch.group_count_x, command.data.dispatch.group_count_y, command.data.dispatch.group_count_z) threadsPerThreadgroup:currentPipeline->threadGroupSize];
                }
                    break;
            }
        }
        
        if(renderEncoder != nil)
            [renderEncoder endEncoding];
        
        if(blitEncoder != nil)
            [blitEncoder endEncoding];
        
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull) {
            for(auto [i, buffer] : utility::enumerate(command_buffers)) {
                if(buffer == command_buffer)
                    free_command_buffers[i] = true;
            }
        }];
        
        if(window != -1) {
            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];

            currentFrameIndex = (currentFrameIndex + 1) % 3;
        } else {
            [commandBuffer commit];
        }
    }
}

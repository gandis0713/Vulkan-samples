#include "webgpu_render_pipeline.h"

#include "webgpu_device.h"
#include "webgpu_pipeline_layout.h"
#include "webgpu_shader_module.h"
#include "webgpu_texture.h"

#include <cstring>

namespace jipu
{

WebGPURenderPipeline* WebGPURenderPipeline::create(WebGPUDevice* wgpuDevice, WGPURenderPipelineDescriptor const* descriptor)
{
    RenderPipelineDescriptor pipelineDescriptor{};

    // pipeline layout
    {
        pipelineDescriptor.layout = reinterpret_cast<WebGPUPipelineLayout*>(descriptor->layout)->getPipelineLayout();
    }

    // input assembly
    {
        pipelineDescriptor.inputAssembly.topology = WGPUToPrimitiveTopology(descriptor->primitive.topology);
    }

    // vertex stage
    {
        VertexStage& vertexStage = pipelineDescriptor.vertex;
        // vertex stage: layout
        {
            for (auto i = 0; i < descriptor->vertex.bufferCount; ++i)
            {
                const auto buffer = descriptor->vertex.buffers[i];

                VertexInputLayout layout{};
                layout.mode = WPGUToVertexMode(buffer.stepMode);
                layout.stride = buffer.arrayStride;

                for (auto j = 0; j < buffer.attributeCount; ++j)
                {
                    auto const& attribute = buffer.attributes[j];
                    VertexAttribute vertexAttribute{};
                    vertexAttribute.format = WGPUToVertexFormat(attribute.format);
                    vertexAttribute.offset = attribute.offset;
                    vertexAttribute.location = attribute.shaderLocation;
                    vertexAttribute.slot = i; // vertex binding index

                    layout.attributes.push_back(vertexAttribute);
                }

                vertexStage.layouts.push_back(layout);
            }
        }

        vertexStage.entryPoint = std::string(descriptor->vertex.entryPoint.data,
                                             descriptor->vertex.entryPoint.length != WGPU_STRLEN ? descriptor->vertex.entryPoint.length : strlen(descriptor->vertex.entryPoint.data));
        vertexStage.shaderModule = reinterpret_cast<WebGPUShaderModule*>(descriptor->vertex.module)->getShaderModule();
    }

    // rasterization stage
    {
        RasterizationStage& rasterization = pipelineDescriptor.rasterization;
        rasterization.cullMode = WGPUToCullMode(descriptor->primitive.cullMode);
        rasterization.frontFace = WGPUToFrontFace(descriptor->primitive.frontFace);
        rasterization.sampleCount = descriptor->multisample.count;
    }

    // fragment stage
    {
        FragmentStage& fragmentStage = pipelineDescriptor.fragment;
        // fragment stage: target
        {
            for (auto i = 0; i < descriptor->fragment->targetCount; ++i)
            {
                auto const wgpuTarget = descriptor->fragment->targets[i];
                FragmentStage::Target target{};
                target.format = WGPUToTextureFormat(wgpuTarget.format);

                if (wgpuTarget.blend)
                {
                    BlendState blendState{};
                    blendState.color.srcFactor = WGPUToBlendFactor(wgpuTarget.blend->color.srcFactor);
                    blendState.color.dstFactor = WGPUToBlendFactor(wgpuTarget.blend->color.dstFactor);
                    blendState.color.operation = WGPUToBlendOperation(wgpuTarget.blend->color.operation);
                    blendState.alpha.srcFactor = WGPUToBlendFactor(wgpuTarget.blend->alpha.srcFactor);
                    blendState.alpha.dstFactor = WGPUToBlendFactor(wgpuTarget.blend->alpha.dstFactor);
                    blendState.alpha.operation = WGPUToBlendOperation(wgpuTarget.blend->alpha.operation);

                    target.blend = blendState;
                }

                fragmentStage.targets.push_back(target);
            }
        }

        fragmentStage.entryPoint = std::string(descriptor->fragment->entryPoint.data,
                                               descriptor->fragment->entryPoint.length != WGPU_STRLEN ? descriptor->fragment->entryPoint.length : strlen(descriptor->fragment->entryPoint.data));
        fragmentStage.shaderModule = reinterpret_cast<WebGPUShaderModule*>(descriptor->fragment->module)->getShaderModule();
    }

    // depth/stencil stage
    {
        if (descriptor->depthStencil)
        {
            DepthStencilStage depthStencilStage{};
            depthStencilStage.format = WGPUToTextureFormat(descriptor->depthStencil->format);
            depthStencilStage.depthWriteEnabled = static_cast<bool>(descriptor->depthStencil->depthWriteEnabled);
            depthStencilStage.depthCompareFunction = WGPUToCompareFunction(descriptor->depthStencil->depthCompare);

            pipelineDescriptor.depthStencil = depthStencilStage;
        }
    }

    auto device = wgpuDevice->getDevice();
    auto renderPipeline = device->createRenderPipeline(pipelineDescriptor);

    return new WebGPURenderPipeline(wgpuDevice, std::move(renderPipeline), descriptor);
}

WebGPURenderPipeline::WebGPURenderPipeline(WebGPUDevice* wgpuDevice, std::unique_ptr<RenderPipeline> pipeline, WGPURenderPipelineDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_pipeline(std::move(pipeline))
{
}

RenderPipeline* WebGPURenderPipeline::getRenderPipeline() const
{
    return m_pipeline.get();
}

// Convert from JIPU to WebGPU
WGPUVertexFormat ToWGPUVertexFormat(VertexFormat format)
{
    switch (format)
    {
    case VertexFormat::kUint8x2:
        return WGPUVertexFormat_Uint8x2;
    case VertexFormat::kUint8x4:
        return WGPUVertexFormat_Uint8x4;
    case VertexFormat::kSint8x2:
        return WGPUVertexFormat_Sint8x2;
    case VertexFormat::kSint8x4:
        return WGPUVertexFormat_Sint8x4;
    case VertexFormat::kUnorm8x2:
        return WGPUVertexFormat_Unorm8x2;
    case VertexFormat::kUnorm8x4:
        return WGPUVertexFormat_Unorm8x4;
    case VertexFormat::kSnorm8x2:
        return WGPUVertexFormat_Snorm8x2;
    case VertexFormat::kSnorm8x4:
        return WGPUVertexFormat_Snorm8x4;
    case VertexFormat::kUint16x2:
        return WGPUVertexFormat_Uint16x2;
    case VertexFormat::kUint16x4:
        return WGPUVertexFormat_Uint16x4;
    case VertexFormat::kSint16x2:
        return WGPUVertexFormat_Sint16x2;
    case VertexFormat::kSint16x4:
        return WGPUVertexFormat_Sint16x4;
    case VertexFormat::kUnorm16x2:
        return WGPUVertexFormat_Unorm16x2;
    case VertexFormat::kUnorm16x4:
        return WGPUVertexFormat_Unorm16x4;
    case VertexFormat::kSnorm16x2:
        return WGPUVertexFormat_Snorm16x2;
    case VertexFormat::kSnorm16x4:
        return WGPUVertexFormat_Snorm16x4;
    case VertexFormat::kFloat16x2:
        return WGPUVertexFormat_Float16x2;
    case VertexFormat::kFloat16x4:
        return WGPUVertexFormat_Float16x4;
    case VertexFormat::kFloat32:
        return WGPUVertexFormat_Float32;
    case VertexFormat::kFloat32x2:
        return WGPUVertexFormat_Float32x2;
    case VertexFormat::kFloat32x3:
        return WGPUVertexFormat_Float32x3;
    case VertexFormat::kFloat32x4:
        return WGPUVertexFormat_Float32x4;
    case VertexFormat::kUint32:
        return WGPUVertexFormat_Uint32;
    case VertexFormat::kUint32x2:
        return WGPUVertexFormat_Uint32x2;
    case VertexFormat::kUint32x3:
        return WGPUVertexFormat_Uint32x3;
    case VertexFormat::kUint32x4:
        return WGPUVertexFormat_Uint32x4;
    case VertexFormat::kSint32:
        return WGPUVertexFormat_Sint32;
    case VertexFormat::kSint32x2:
        return WGPUVertexFormat_Sint32x2;
    case VertexFormat::kSint32x3:
        return WGPUVertexFormat_Sint32x3;
    case VertexFormat::kSint32x4:
        return WGPUVertexFormat_Sint32x4;
    case VertexFormat::kUnorm10_10_10_2:
        return WGPUVertexFormat_Unorm10_10_10_2;
    default:
        throw std::runtime_error("vertex format is not supported.");
    }
}

WGPUVertexStepMode ToWGPUVertexStepMode(VertexMode mode)
{
    switch (mode)
    {
    case VertexMode::kVertex:
        return WGPUVertexStepMode_Vertex;
    case VertexMode::kVertexBufferNotUsed:
        return WGPUVertexStepMode_VertexBufferNotUsed;
    case VertexMode::kInstance:
        return WGPUVertexStepMode_Instance;
    default:
        throw std::runtime_error("vertex step mode is not supported.");
    }
}

WGPUPrimitiveTopology ToWGPUPrimitiveTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::kPointList:
        return WGPUPrimitiveTopology_PointList;
    case PrimitiveTopology::kLineStrip:
        return WGPUPrimitiveTopology_LineStrip;
    case PrimitiveTopology::kLineList:
        return WGPUPrimitiveTopology_LineList;
    case PrimitiveTopology::kTriangleStrip:
        return WGPUPrimitiveTopology_TriangleStrip;
    case PrimitiveTopology::kTriangleList:
        return WGPUPrimitiveTopology_TriangleList;
    default:
        throw std::runtime_error("primitive topology is not supported.");
    }
}

WGPUCullMode ToWGPUCullMode(CullMode mode)
{
    switch (mode)
    {
    case CullMode::kNone:
        return WGPUCullMode_None;
    case CullMode::kFront:
        return WGPUCullMode_Front;
    case CullMode::kBack:
        return WGPUCullMode_Back;
    default:
        throw std::runtime_error("cull mode is not supported.");
    }
}

WGPUFrontFace ToWGPUFrontFace(FrontFace face)
{
    switch (face)
    {
    case FrontFace::kCounterClockwise:
        return WGPUFrontFace_CCW;
    case FrontFace::kClockwise:
        return WGPUFrontFace_CW;
    default:
        throw std::runtime_error("front face is not supported.");
    }
}

WGPUBlendFactor ToWGPUBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
    case BlendFactor::kZero:
        return WGPUBlendFactor_Zero;
    case BlendFactor::kOne:
        return WGPUBlendFactor_One;
    case BlendFactor::kSrcColor:
        return WGPUBlendFactor_Src;
    case BlendFactor::kOneMinusSrcColor:
        return WGPUBlendFactor_OneMinusSrc;
    case BlendFactor::kSrcAlpha:
        return WGPUBlendFactor_SrcAlpha;
    case BlendFactor::kOneMinusSrcAlpha:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case BlendFactor::kDstColor:
        return WGPUBlendFactor_Dst;
    case BlendFactor::kOneMinusDstColor:
        return WGPUBlendFactor_OneMinusDst;
    case BlendFactor::kDstAlpha:
        return WGPUBlendFactor_DstAlpha;
    case BlendFactor::kOneMinusDstAlpha:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case BlendFactor::kSrcAlphaSaturated:
        return WGPUBlendFactor_SrcAlphaSaturated;
    case BlendFactor::kConstantColor:
        return WGPUBlendFactor_Constant;
    case BlendFactor::kOneMinusConstantColor:
        return WGPUBlendFactor_OneMinusConstant;
    case BlendFactor::kSrc1Color:
        return WGPUBlendFactor_Src1;
    case BlendFactor::kOneMinusSrc1Color:
        return WGPUBlendFactor_OneMinusSrc1;
    case BlendFactor::kSrc1Alpha:
        return WGPUBlendFactor_Src1Alpha;
    case BlendFactor::kOneMinusSrc1Alpha:
        return WGPUBlendFactor_OneMinusSrc1Alpha;
    default:
        throw std::runtime_error("blend factor is not supported.");
    }
}

WGPUBlendOperation ToWGPUBlendOperation(BlendOperation operation)
{
    switch (operation)
    {
    case BlendOperation::kAdd:
        return WGPUBlendOperation_Add;
    case BlendOperation::kSubtract:
        return WGPUBlendOperation_Subtract;
    case BlendOperation::kReversSubtract:
        return WGPUBlendOperation_ReverseSubtract;
    case BlendOperation::kMin:
        return WGPUBlendOperation_Min;
    case BlendOperation::kMax:
        return WGPUBlendOperation_Max;
    default:
        throw std::runtime_error("blend operation is not supported.");
    }
}

WGPUCompareFunction ToWGPUCompareFunction(CompareFunction function)
{
    switch (function)
    {
    case CompareFunction::kNever:
        return WGPUCompareFunction_Never;
    case CompareFunction::kLess:
        return WGPUCompareFunction_Less;
    case CompareFunction::kEqual:
        return WGPUCompareFunction_Equal;
    case CompareFunction::kLessEqual:
        return WGPUCompareFunction_LessEqual;
    case CompareFunction::kGreater:
        return WGPUCompareFunction_Greater;
    case CompareFunction::kNotEqual:
        return WGPUCompareFunction_NotEqual;
    case CompareFunction::kGreaterEqual:
        return WGPUCompareFunction_GreaterEqual;
    case CompareFunction::kAlways:
        return WGPUCompareFunction_Always;
    default:
        throw std::runtime_error("compare function is not supported.");
    }
}

// Convert from WebGPU to JIPU
VertexFormat WGPUToVertexFormat(WGPUVertexFormat format)
{
    switch (format)
    {
    case WGPUVertexFormat_Uint8x2:
        return VertexFormat::kUint8x2;
    case WGPUVertexFormat_Uint8x4:
        return VertexFormat::kUint8x4;
    case WGPUVertexFormat_Sint8x2:
        return VertexFormat::kSint8x2;
    case WGPUVertexFormat_Sint8x4:
        return VertexFormat::kSint8x4;
    case WGPUVertexFormat_Unorm8x2:
        return VertexFormat::kUnorm8x2;
    case WGPUVertexFormat_Unorm8x4:
        return VertexFormat::kUnorm8x4;
    case WGPUVertexFormat_Snorm8x2:
        return VertexFormat::kSnorm8x2;
    case WGPUVertexFormat_Snorm8x4:
        return VertexFormat::kSnorm8x4;
    case WGPUVertexFormat_Uint16x2:
        return VertexFormat::kUint16x2;
    case WGPUVertexFormat_Uint16x4:
        return VertexFormat::kUint16x4;
    case WGPUVertexFormat_Sint16x2:
        return VertexFormat::kSint16x2;
    case WGPUVertexFormat_Sint16x4:
        return VertexFormat::kSint16x4;
    case WGPUVertexFormat_Unorm16x2:
        return VertexFormat::kUnorm16x2;
    case WGPUVertexFormat_Unorm16x4:
        return VertexFormat::kUnorm16x4;
    case WGPUVertexFormat_Snorm16x2:
        return VertexFormat::kSnorm16x2;
    case WGPUVertexFormat_Snorm16x4:
        return VertexFormat::kSnorm16x4;
    case WGPUVertexFormat_Float16x2:
        return VertexFormat::kFloat16x2;
    case WGPUVertexFormat_Float16x4:
        return VertexFormat::kFloat16x4;
    case WGPUVertexFormat_Float32:
        return VertexFormat::kFloat32;
    case WGPUVertexFormat_Float32x2:
        return VertexFormat::kFloat32x2;
    case WGPUVertexFormat_Float32x3:
        return VertexFormat::kFloat32x3;
    case WGPUVertexFormat_Float32x4:
        return VertexFormat::kFloat32x4;
    case WGPUVertexFormat_Uint32:
        return VertexFormat::kUint32;
    case WGPUVertexFormat_Uint32x2:
        return VertexFormat::kUint32x2;
    case WGPUVertexFormat_Uint32x3:
        return VertexFormat::kUint32x3;
    case WGPUVertexFormat_Uint32x4:
        return VertexFormat::kUint32x4;
    case WGPUVertexFormat_Sint32:
        return VertexFormat::kSint32;
    case WGPUVertexFormat_Sint32x2:
        return VertexFormat::kSint32x2;
    case WGPUVertexFormat_Sint32x3:
        return VertexFormat::kSint32x3;
    case WGPUVertexFormat_Sint32x4:
        return VertexFormat::kSint32x4;
    case WGPUVertexFormat_Unorm10_10_10_2:
        return VertexFormat::kUnorm10_10_10_2;
    default:
        throw std::runtime_error("vertex format is not supported.");
    }
}

VertexMode WPGUToVertexMode(WGPUVertexStepMode mode)
{
    switch (mode)
    {
    case WGPUVertexStepMode_Vertex:
        return VertexMode::kVertex;
    case WGPUVertexStepMode_VertexBufferNotUsed:
        return VertexMode::kVertexBufferNotUsed;
    case WGPUVertexStepMode_Instance:
        return VertexMode::kInstance;
    default:
        throw std::runtime_error("vertex step mode is not supported.");
    }
}

PrimitiveTopology WGPUToPrimitiveTopology(WGPUPrimitiveTopology topology)
{
    switch (topology)
    {
    case WGPUPrimitiveTopology_PointList:
        return PrimitiveTopology::kPointList;
    case WGPUPrimitiveTopology_LineStrip:
        return PrimitiveTopology::kLineStrip;
    case WGPUPrimitiveTopology_LineList:
        return PrimitiveTopology::kLineList;
    case WGPUPrimitiveTopology_TriangleStrip:
        return PrimitiveTopology::kTriangleStrip;
    case WGPUPrimitiveTopology_TriangleList:
        return PrimitiveTopology::kTriangleList;
    default:
        throw std::runtime_error("primitive topology is not supported.");
    }
}

CullMode WGPUToCullMode(WGPUCullMode mode)
{
    switch (mode)
    {
    case WGPUCullMode_None:
        return CullMode::kNone;
    case WGPUCullMode_Front:
        return CullMode::kFront;
    case WGPUCullMode_Back:
        return CullMode::kBack;
    default:
        throw std::runtime_error("cull mode is not supported.");
    }
}

FrontFace WGPUToFrontFace(WGPUFrontFace face)
{
    switch (face)
    {
    case WGPUFrontFace_CCW:
        return FrontFace::kCounterClockwise;
    case WGPUFrontFace_CW:
        return FrontFace::kClockwise;
    default:
        throw std::runtime_error("front face is not supported.");
    }
}

BlendFactor WGPUToBlendFactor(WGPUBlendFactor factor)
{
    switch (factor)
    {
    case WGPUBlendFactor_Zero:
        return BlendFactor::kZero;
    case WGPUBlendFactor_One:
        return BlendFactor::kOne;
    case WGPUBlendFactor_Src:
        return BlendFactor::kSrcColor;
    case WGPUBlendFactor_OneMinusSrc:
        return BlendFactor::kOneMinusSrcColor;
    case WGPUBlendFactor_SrcAlpha:
        return BlendFactor::kSrcAlpha;
    case WGPUBlendFactor_OneMinusSrcAlpha:
        return BlendFactor::kOneMinusSrcAlpha;
    case WGPUBlendFactor_Dst:
        return BlendFactor::kDstColor;
    case WGPUBlendFactor_OneMinusDst:
        return BlendFactor::kOneMinusDstColor;
    case WGPUBlendFactor_DstAlpha:
        return BlendFactor::kDstAlpha;
    case WGPUBlendFactor_OneMinusDstAlpha:
        return BlendFactor::kOneMinusDstAlpha;
    case WGPUBlendFactor_SrcAlphaSaturated:
        return BlendFactor::kSrcAlphaSaturated;
    case WGPUBlendFactor_Constant:
        return BlendFactor::kConstantColor;
    case WGPUBlendFactor_OneMinusConstant:
        return BlendFactor::kOneMinusConstantColor;
    case WGPUBlendFactor_Src1:
        return BlendFactor::kSrc1Color;
    case WGPUBlendFactor_OneMinusSrc1:
        return BlendFactor::kOneMinusSrc1Color;
    case WGPUBlendFactor_Src1Alpha:
        return BlendFactor::kSrc1Alpha;
    case WGPUBlendFactor_OneMinusSrc1Alpha:
        return BlendFactor::kOneMinusSrc1Alpha;
    default:
        throw std::runtime_error("blend factor is not supported.");
    }
}

BlendOperation WGPUToBlendOperation(WGPUBlendOperation operation)
{
    switch (operation)
    {
    case WGPUBlendOperation_Add:
        return BlendOperation::kAdd;
    case WGPUBlendOperation_Subtract:
        return BlendOperation::kSubtract;
    case WGPUBlendOperation_ReverseSubtract:
        return BlendOperation::kReversSubtract;
    case WGPUBlendOperation_Min:
        return BlendOperation::kMin;
    case WGPUBlendOperation_Max:
        return BlendOperation::kMax;
    default:
        throw std::runtime_error("blend operation is not supported.");
    }
}

CompareFunction WGPUToCompareFunction(WGPUCompareFunction function)
{
    switch (function)
    {
    case WGPUCompareFunction_Never:
        return CompareFunction::kNever;
    case WGPUCompareFunction_Less:
        return CompareFunction::kLess;
    case WGPUCompareFunction_Equal:
        return CompareFunction::kEqual;
    case WGPUCompareFunction_LessEqual:
        return CompareFunction::kLessEqual;
    case WGPUCompareFunction_Greater:
        return CompareFunction::kGreater;
    case WGPUCompareFunction_NotEqual:
        return CompareFunction::kNotEqual;
    case WGPUCompareFunction_GreaterEqual:
        return CompareFunction::kGreaterEqual;
    case WGPUCompareFunction_Always:
        return CompareFunction::kAlways;
    default:
        throw std::runtime_error("compare function is not supported.");
    }
}

} // namespace jipu
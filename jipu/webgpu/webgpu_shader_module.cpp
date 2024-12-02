#include "webgpu_shader_module.h"

#include "webgpu_device.h"

#define TINT_BUILD_SPV_READER 1
#define TINT_BUILD_WGSL_READER 1
#define TINT_BUILD_SPV_WRITER 1
#define TINT_BUILD_WGSL_WRITER 1

#undef TINT_BUILD_GLSL_WRITER
#undef TINT_BUILD_HLSL_WRITER
#undef TINT_BUILD_MSL_WRITER

#include "tint/tint.h"

namespace jipu
{

WebGPUShaderModule* WebGPUShaderModule::create(WebGPUDevice* wgpuDevice, WGPUShaderModuleDescriptor const* descriptor)
{
    std::unique_ptr<ShaderModule> shaderModule = nullptr;

    const WGPUChainedStruct* current = descriptor->nextInChain;
    while (current)
    {
        switch (current->sType)
        {
        case WGPUSType_ShaderSourceWGSL: {

            WGPUShaderModuleWGSLDescriptor const* wgslDescriptor = reinterpret_cast<WGPUShaderModuleWGSLDescriptor const*>(current);

            auto tintFile = std::make_unique<tint::Source::File>("", wgslDescriptor->code.data);

            tint::wgsl::reader::Options wgslReaderOptions;
            {
                wgslReaderOptions.allowed_features = tint::wgsl::AllowedFeatures::Everything();
            }

            tint::Program tintProgram = tint::wgsl::reader::Parse(tintFile.get(), wgslReaderOptions);

            auto ir = tint::wgsl::reader::ProgramToLoweredIR(tintProgram);
            if (ir != tint::Success)
            {
                std::string msg = ir.Failure().reason.Str();
                throw std::runtime_error(msg.c_str());
            }

            tint::spirv::writer::Options sprivWriterOptions;
            {
                sprivWriterOptions.bindings = {};

                sprivWriterOptions.disable_robustness = false;
                sprivWriterOptions.disable_image_robustness = true;
                sprivWriterOptions.disable_runtime_sized_array_index_clamping = true;

                sprivWriterOptions.use_zero_initialize_workgroup_memory_extension = true;
                sprivWriterOptions.use_storage_input_output_16 = true;
                sprivWriterOptions.emit_vertex_point_size = false;
                sprivWriterOptions.clamp_frag_depth = false;

                sprivWriterOptions.experimental_require_subgroup_uniform_control_flow = false;
                sprivWriterOptions.use_vulkan_memory_model = false;

                sprivWriterOptions.polyfill_dot_4x8_packed = false;
                sprivWriterOptions.disable_polyfill_integer_div_mod = false;

                sprivWriterOptions.disable_workgroup_init = false;
            }

            auto tintResult = tint::spirv::writer::Generate(ir.Get(), sprivWriterOptions);
            if (tintResult != tint::Success)
            {
                std::string msg = tintResult.Failure().reason.Str();
                throw std::runtime_error(msg.c_str());
            }

            std::vector<uint32_t> spriv = std::move(tintResult.Get().spirv);

            ShaderModuleDescriptor shaderModuleDescriptor{};
            shaderModuleDescriptor.code = reinterpret_cast<const char*>(spriv.data());
            shaderModuleDescriptor.codeSize = spriv.size() * sizeof(uint32_t);

            auto device = wgpuDevice->getDevice();
            shaderModule = device->createShaderModule(shaderModuleDescriptor);
        }
        break;
        case WGPUSType_ShaderSourceSPIRV: {
            WGPUShaderModuleSPIRVDescriptor const* spirvDescriptor = reinterpret_cast<WGPUShaderModuleSPIRVDescriptor const*>(current);

            ShaderModuleDescriptor shaderModuleDescriptor{};
            shaderModuleDescriptor.code = reinterpret_cast<const char*>(spirvDescriptor->code);
            shaderModuleDescriptor.codeSize = spirvDescriptor->codeSize;

            auto device = wgpuDevice->getDevice();
            shaderModule = device->createShaderModule(shaderModuleDescriptor);
        }
        break;
        default:
            throw std::runtime_error("Unsupported WGPUShaderModuleDescriptor type");
        }

        current = current->next;
    }

    return new WebGPUShaderModule(wgpuDevice, std::move(shaderModule), descriptor);
}

WebGPUShaderModule::WebGPUShaderModule(WebGPUDevice* wgpuDevice, std::unique_ptr<ShaderModule> shaderModule, WGPUShaderModuleDescriptor const* descriptor)
    : m_wgpuDevice(wgpuDevice)
    , m_descriptor(*descriptor)
    , m_shaderModule(std::move(shaderModule))
{
}

ShaderModule* WebGPUShaderModule::getShaderModule() const
{
    return m_shaderModule.get();
}

} // namespace jipu
#include "webgpu_shader_module.h"

#include "webgpu_device.h"

#define TINT_BUILD_SPV_READER 1
#define TINT_BUILD_WGSL_READER 1
#define TINT_BUILD_SPV_WRITER 1
#define TINT_BUILD_WGSL_WRITER 1

#include "tint/tint.h"

namespace jipu
{

WebGPUShaderModule* WebGPUShaderModule::create(WebGPUDevice* wgpuDevice, WGPUShaderModuleDescriptor const* descriptor)
{
    ShaderModuleDescriptor shaderModuleDescriptor{};

    const WGPUChainedStruct* current = descriptor->nextInChain;
    while (current)
    {
        switch (current->sType)
        {
        case WGPUSType_ShaderSourceWGSL: {

            WGPUShaderModuleWGSLDescriptor const* wgslDescriptor = reinterpret_cast<WGPUShaderModuleWGSLDescriptor const*>(current);

            auto tintFile = std::make_unique<tint::Source::File>("", wgslDescriptor->code.data);

            tint::wgsl::reader::Options wgslReaderOptions;
            tint::Program tintProgram = tint::wgsl::reader::Parse(tintFile.get(), wgslReaderOptions);

            auto ir = tint::wgsl::reader::ProgramToLoweredIR(tintProgram);
            if (ir != tint::Success)
            {
                std::string msg = ir.Failure().reason.Str();
                throw std::runtime_error(msg.c_str());
            }

            tint::spirv::writer::Options sprivWriterOptions;
            auto tintResult = tint::spirv::writer::Generate(ir.Get(), sprivWriterOptions);
            if (tintResult != tint::Success)
            {
                std::string msg = tintResult.Failure().reason.Str();
                throw std::runtime_error(msg.c_str());
            }

            std::vector<uint32_t> spriv = std::move(tintResult.Get().spirv);

            shaderModuleDescriptor.code = reinterpret_cast<const char*>(spriv.data());
            shaderModuleDescriptor.codeSize = spriv.size() * sizeof(uint32_t);
        }
        break;
        case WGPUSType_ShaderSourceSPIRV: {
            WGPUShaderModuleSPIRVDescriptor const* spirvDescriptor = reinterpret_cast<WGPUShaderModuleSPIRVDescriptor const*>(current);
            shaderModuleDescriptor.code = reinterpret_cast<const char*>(spirvDescriptor->code);
            shaderModuleDescriptor.codeSize = spirvDescriptor->codeSize;
        }
        break;
        default:
            throw std::runtime_error("Unsupported WGPUShaderModuleDescriptor type");
        }

        current = current->next;
    }

    auto device = wgpuDevice->getDevice();
    auto shaderModule = device->createShaderModule(shaderModuleDescriptor);

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
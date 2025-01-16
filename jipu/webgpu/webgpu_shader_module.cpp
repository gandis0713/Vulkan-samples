#include "webgpu_shader_module.h"

#include "webgpu_device.h"

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

            ShaderModuleDescriptor shaderModuleDescriptor{};
            shaderModuleDescriptor.type = ShaderModuleType::kWGSL;

            shaderModuleDescriptor.code = std::string_view(wgslDescriptor->code.data,
                                                           wgslDescriptor->code.length != WGPU_STRLEN ? wgslDescriptor->code.length : strlen(wgslDescriptor->code.data));

            auto device = wgpuDevice->getDevice();
            shaderModule = device->createShaderModule(shaderModuleDescriptor);
        }
        break;
        case WGPUSType_ShaderSourceSPIRV: {
            WGPUShaderModuleSPIRVDescriptor const* spirvDescriptor = reinterpret_cast<WGPUShaderModuleSPIRVDescriptor const*>(current);

            std::vector<char> spirvData(spirvDescriptor->codeSize * sizeof(uint32_t));
            std::memcpy(spirvData.data(), spirvDescriptor->code, spirvData.size());

            ShaderModuleDescriptor shaderModuleDescriptor{};
            shaderModuleDescriptor.type = ShaderModuleType::kSPIRV;
            shaderModuleDescriptor.code = std::string_view(spirvData.data(), spirvData.size());

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
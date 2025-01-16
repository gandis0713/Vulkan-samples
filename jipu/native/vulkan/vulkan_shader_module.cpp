#include "vulkan_shader_module.h"

#include "jipu/common/hash.h"
#include "vulkan_api.h"
#include "vulkan_device.h"

#include <fmt/format.h>
#include <stdexcept>

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

static size_t getHash(const VulkanShaderModuleInfo& info)
{
    size_t hash = 0;

    combineHash(hash, info.type);
    combineHash(hash, info.code);

    return hash;
}
static tint::spirv::writer::Options getSprivWriterOptions()
{
    tint::spirv::writer::Options sprivWriterOptions;
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

    return sprivWriterOptions;
}

VulkanShaderModule::VulkanShaderModule(VulkanDevice* device, const ShaderModuleDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    m_metaData.info = VulkanShaderModuleInfo{
        .type = descriptor.type,
        .code = std::string(descriptor.code)
    };
    m_metaData.hash = getHash(m_metaData.info);
}

VulkanShaderModule::~VulkanShaderModule()
{
    m_device->getDeleter()->safeDestroy(m_shaderModule);
}

VkShaderModule VulkanShaderModule::getVkShaderModule(const std::string_view entryPoint) const
{
    return m_device->getShaderModuleCache()->getVkShaderModule(m_metaData, entryPoint);
}

const VulkanShaderModuleMetaData& VulkanShaderModule::getMetaData() const
{
    return m_metaData;
}

// VulkanShaderModuleCache

VulkanShaderModuleCache::VulkanShaderModuleCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanShaderModuleCache::~VulkanShaderModuleCache()
{
    clear();
}

VkShaderModule VulkanShaderModuleCache::getVkShaderModule(const VulkanShaderModuleMetaData& metaData, const std::string_view entryPoint)
{
    auto hash = metaData.hash;
    if (!m_shaderModuleCache.contains(hash))
    {
        hash = getHash(metaData.info);
        if (!m_shaderModuleCache.contains(hash))
        {
            m_shaderModuleCache[hash] = {};
        }
    }

    auto& shaderModules = m_shaderModuleCache[hash];
    if (shaderModules.contains(entryPoint))
    {
        return shaderModules.at(entryPoint);
    }

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    switch (metaData.info.type)
    {
    case ShaderModuleType::kWGSL:
        shaderModule = createWGSLShaderModule(metaData, entryPoint);
        break;
    case ShaderModuleType::kSPIRV:
        shaderModule = createSPIRVShaderModule(metaData);
        break;
    default:
        throw std::runtime_error("Unsupported ShaderModuleType");
        break;
    }

    shaderModules.insert({ entryPoint, shaderModule });

    return shaderModule;
}

void VulkanShaderModuleCache::clear()
{
    for (auto& [_, shaderModules] : m_shaderModuleCache)
    {
        for (auto& [_, shaderModule] : shaderModules)
        {
            m_device->getDeleter()->safeDestroy(shaderModule);
        }
    }

    m_shaderModuleCache.clear();
}

VkShaderModule VulkanShaderModuleCache::createWGSLShaderModule(const VulkanShaderModuleMetaData& metaData, const std::string_view entryPoint)
{
    auto tintFile = std::make_unique<tint::Source::File>("", std::string_view(metaData.info.code));

    tint::wgsl::reader::Options wgslReaderOptions;
    {
        wgslReaderOptions.allowed_features = tint::wgsl::AllowedFeatures::Everything();
    }

    tint::Program tintProgram = tint::wgsl::reader::Parse(tintFile.get(), wgslReaderOptions);

    tint::ast::transform::Manager transformManager;
    tint::ast::transform::DataMap transformInputs;

    // Many Vulkan drivers can't handle multi-entrypoint shader modules.
    // Run before the renamer so that the entry point name matches `entryPointName` still.
    transformManager.append(std::make_unique<tint::ast::transform::SingleEntryPoint>());
    transformInputs.Add<tint::ast::transform::SingleEntryPoint::Config>(
        std::string(entryPoint));

    tint::ast::transform::DataMap transform_outputs;
    tint::Program tintProgram2 = transformManager.Run(tintProgram, transformInputs, transform_outputs);

    auto ir = tint::wgsl::reader::ProgramToLoweredIR(tintProgram2);
    if (ir != tint::Success)
    {
        std::string msg = ir.Failure().reason.Str();
        throw std::runtime_error(msg.c_str());
    }

    tint::spirv::writer::Options sprivWriterOptions = getSprivWriterOptions();
    auto tintResult = tint::spirv::writer::Generate(ir.Get(), sprivWriterOptions);
    if (tintResult != tint::Success)
    {
        std::string msg = tintResult.Failure().reason.Str();
        throw std::runtime_error(msg.c_str());
    }

    std::vector<uint32_t> spirv = std::move(tintResult.Get().spirv);
    std::vector<char> spirvData(spirv.size() * sizeof(uint32_t));
    std::memcpy(spirvData.data(), spirv.data(), spirvData.size());

    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = spirvData.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(spirvData.data());

    auto vulkanDevice = downcast(m_device);

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vulkanDevice->vkAPI.CreateShaderModule(vulkanDevice->getVkDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create shader module. [Result: {}]", static_cast<int32_t>(result)));
    }

    return shaderModule;
}

VkShaderModule VulkanShaderModuleCache::createSPIRVShaderModule(const VulkanShaderModuleMetaData& metaData)
{
    auto vulkanDevice = downcast(m_device);

    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = metaData.info.code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(metaData.info.code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vulkanDevice->vkAPI.CreateShaderModule(vulkanDevice->getVkDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create shader module. [Result: {}]", static_cast<int32_t>(result)));
    }

    return shaderModule;
}

} // namespace jipu
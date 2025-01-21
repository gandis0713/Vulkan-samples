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

#define SAVE_SPIRV_FOR_TEST
#if defined(SAVE_SPIRV_FOR_TEST) && defined(__APPLE__)
#include "src/tint/lang/wgsl/ast/transform/canonicalize_entry_point_io.h"
#include <fstream>

uint32_t shaderCount = 0;
void saveSPIRV(const std::vector<uint32_t>& spriv, const std::string& dir)
{
    auto save = [&](const std::vector<uint32_t>& spirv, const std::string& filename) -> void {
        std::ofstream file(filename, std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        file.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
        file.close();
    };

    save(spriv, dir + "test" + std::to_string(shaderCount) + ".spv");
    shaderCount++;
}
#endif

namespace jipu
{

size_t getHash(const VulkanShaderModuleInfo& info)
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
    m_info = VulkanShaderModuleInfo{
        .type = descriptor.type,
        .code = std::string(descriptor.code)
    };
}

VulkanShaderModule::~VulkanShaderModule()
{
    m_device->getDeleter()->safeDestroy(m_shaderModule);
}

VkShaderModule VulkanShaderModule::getVkShaderModule(const VulkanPipelineLayoutMetaData& layoutMetaData,
                                                     std::string_view entryPoint) const
{
    VulkanShaderModuleMetaData metaData{
        .modulInfo = m_info,
        .layoutInfo = layoutMetaData.info,
        .entryPoint = std::string(entryPoint)
    };

    return m_device->getShaderModuleCache()->getVkShaderModule(metaData);
}

const VulkanShaderModuleInfo& VulkanShaderModule::getInfo() const
{
    return m_info;
}

// VulkanShaderModuleCache

size_t VulkanShaderModuleCache::Functor::operator()(const VulkanShaderModuleMetaData& metaData) const
{
    size_t hash = 0;

    combineHash(hash, metaData.entryPoint);

    combineHash(hash, metaData.modulInfo.type);
    combineHash(hash, metaData.modulInfo.code);

    for (const auto& bindGroupLayoutMetaData : metaData.layoutInfo.bindGroupLayoutMetaDatas)
    {
        for (const auto& buffer : bindGroupLayoutMetaData.info.buffers)
        {
            combineHash(hash, buffer.dynamicOffset);
            combineHash(hash, buffer.index);
            combineHash(hash, buffer.stages);
            combineHash(hash, buffer.type);
        }

        for (const auto& sampler : bindGroupLayoutMetaData.info.samplers)
        {
            combineHash(hash, sampler.index);
            combineHash(hash, sampler.stages);
        }

        for (const auto& texture : bindGroupLayoutMetaData.info.textures)
        {
            combineHash(hash, texture.index);
            combineHash(hash, texture.stages);
        }

        for (const auto& storageTexture : bindGroupLayoutMetaData.info.storageTextures)
        {
            combineHash(hash, storageTexture.index);
            combineHash(hash, storageTexture.stages);
            combineHash(hash, storageTexture.type);
        }
    }

    return hash;
}

bool VulkanShaderModuleCache::Functor::operator()(const VulkanShaderModuleMetaData& lhs,
                                                  const VulkanShaderModuleMetaData& rhs) const
{
    if (lhs.entryPoint != rhs.entryPoint ||
        lhs.modulInfo.type != rhs.modulInfo.type ||
        lhs.modulInfo.code != rhs.modulInfo.code ||
        lhs.layoutInfo.bindGroupLayoutMetaDatas.size() != rhs.layoutInfo.bindGroupLayoutMetaDatas.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.layoutInfo.bindGroupLayoutMetaDatas.size(); ++i)
    {
        if (lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers.size() != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers.size() ||
            lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers.size() != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers.size() ||
            lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures.size() != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures.size() ||
            lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures.size() != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures.size())
        {
            return false;
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].dynamicOffset != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].dynamicOffset ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].index != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].index ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].stages != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].stages ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].type != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.buffers[j].type)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers[j].index != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers[j].index ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers[j].stages != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.samplers[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures[j].index != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures[j].index ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures[j].stages != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.textures[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].index != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].index ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].stages != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].stages ||
                lhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].type != rhs.layoutInfo.bindGroupLayoutMetaDatas[i].info.storageTextures[j].type)
            {
                return false;
            }
        }
    }

    return true;
}

VulkanShaderModuleCache::VulkanShaderModuleCache(VulkanDevice* device)
    : m_device(device)
{
}

VulkanShaderModuleCache::~VulkanShaderModuleCache()
{
    clear();
}

VkShaderModule VulkanShaderModuleCache::getVkShaderModule(const VulkanShaderModuleMetaData& metaData)
{
    auto it = m_shaderModuleCache.find(metaData);
    if (it != m_shaderModuleCache.end())
    {
        return it->second;
    }

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    switch (metaData.modulInfo.type)
    {
    case ShaderModuleType::kWGSL:
        shaderModule = createWGSLShaderModule(metaData);
        break;
    case ShaderModuleType::kSPIRV:
        shaderModule = createSPIRVShaderModule(metaData);
        break;
    default:
        throw std::runtime_error("Unsupported ShaderModuleType");
        break;
    }

    m_shaderModuleCache.insert({ metaData, shaderModule });

    return shaderModule;
}

void VulkanShaderModuleCache::clear()
{
    for (auto& [_, shaderModule] : m_shaderModuleCache)
    {
        m_device->getDeleter()->safeDestroy(shaderModule);
    }

    m_shaderModuleCache.clear();
}

VkShaderModule VulkanShaderModuleCache::createWGSLShaderModule(const VulkanShaderModuleMetaData& metaData)
{
    auto tintFile = std::make_unique<tint::Source::File>("", std::string_view(metaData.modulInfo.code));

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
        std::string(metaData.entryPoint));

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
#if defined(SAVE_SPIRV_FOR_TEST) && defined(__APPLE__)
    saveSPIRV(spirv, std::string("/Users/changhyun/Desktop/git/github/gandis/jipu/"));
#endif

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
    shaderModuleCreateInfo.codeSize = metaData.modulInfo.code.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(metaData.modulInfo.code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vulkanDevice->vkAPI.CreateShaderModule(vulkanDevice->getVkDevice(), &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(fmt::format("Failed to create shader module. [Result: {}]", static_cast<int32_t>(result)));
    }

    return shaderModule;
}

} // namespace jipu
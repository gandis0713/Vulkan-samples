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

static tint::spirv::writer::Bindings generateBindings(const VulkanPipelineLayoutInfo& layoutInfo)
{
    tint::spirv::writer::Bindings bindings{};

    for (auto groupIndex = 0; groupIndex < layoutInfo.bindGroupLayoutInfos.size(); ++groupIndex)
    {
        const auto& bindGroupLayoutInfo = layoutInfo.bindGroupLayoutInfos[groupIndex];
        for (const auto& buffer : bindGroupLayoutInfo.buffers)
        {
            if (buffer.type == BufferBindingType::kUniform)
            {
                tint::BindingPoint bindingPoint{};
                tint::spirv::writer::binding::Uniform uniform;

                uniform.binding = buffer.index;
                uniform.group = groupIndex;

                bindingPoint.group = uniform.group;
                bindingPoint.binding = uniform.binding;

                bindings.uniform[bindingPoint] = uniform;
            }
        }

        for (const auto& sampler : bindGroupLayoutInfo.samplers)
        {
            tint::BindingPoint bindingPoint{};
            tint::spirv::writer::binding::Sampler samplerBinding;

            samplerBinding.binding = sampler.index;
            samplerBinding.group = groupIndex;

            bindingPoint.group = samplerBinding.group;
            bindingPoint.binding = samplerBinding.binding;

            bindings.sampler[bindingPoint] = samplerBinding;
        }

        for (const auto& texture : bindGroupLayoutInfo.textures)
        {
            tint::BindingPoint bindingPoint{};
            tint::spirv::writer::binding::Texture textureBinding;

            textureBinding.binding = texture.index;
            textureBinding.group = groupIndex;

            bindingPoint.group = textureBinding.group;
            bindingPoint.binding = textureBinding.binding;

            bindings.texture[bindingPoint] = textureBinding;
        }

        for (const auto& storageTexture : bindGroupLayoutInfo.storageTextures)
        {
            tint::BindingPoint bindingPoint{};
            tint::spirv::writer::binding::StorageTexture storageTextureBinding;

            storageTextureBinding.binding = storageTexture.index;
            storageTextureBinding.group = groupIndex;

            bindingPoint.group = storageTextureBinding.group;
            bindingPoint.binding = storageTextureBinding.binding;

            bindings.storage_texture[bindingPoint] = storageTextureBinding;
        }
    }

    return bindings;
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

VkShaderModule VulkanShaderModule::getVkShaderModule(const VulkanPipelineLayoutInfo& layoutInfo,
                                                     std::string_view entryPoint) const
{
    VulkanShaderModuleMetaData metaData{
        .modulInfo = m_info,
        .layoutInfo = layoutInfo,
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

    for (const auto& bindGroupLayoutInfo : metaData.layoutInfo.bindGroupLayoutInfos)
    {
        for (const auto& buffer : bindGroupLayoutInfo.buffers)
        {
            combineHash(hash, buffer.dynamicOffset);
            combineHash(hash, buffer.index);
            combineHash(hash, buffer.stages);
            combineHash(hash, buffer.type);
        }

        for (const auto& sampler : bindGroupLayoutInfo.samplers)
        {
            combineHash(hash, sampler.index);
            combineHash(hash, sampler.stages);
        }

        for (const auto& texture : bindGroupLayoutInfo.textures)
        {
            combineHash(hash, texture.index);
            combineHash(hash, texture.stages);
        }

        for (const auto& storageTexture : bindGroupLayoutInfo.storageTextures)
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
        lhs.layoutInfo.bindGroupLayoutInfos.size() != rhs.layoutInfo.bindGroupLayoutInfos.size())
    {
        return false;
    }

    for (auto i = 0; i < lhs.layoutInfo.bindGroupLayoutInfos.size(); ++i)
    {
        if (lhs.layoutInfo.bindGroupLayoutInfos[i].buffers.size() != rhs.layoutInfo.bindGroupLayoutInfos[i].buffers.size() ||
            lhs.layoutInfo.bindGroupLayoutInfos[i].samplers.size() != rhs.layoutInfo.bindGroupLayoutInfos[i].samplers.size() ||
            lhs.layoutInfo.bindGroupLayoutInfos[i].textures.size() != rhs.layoutInfo.bindGroupLayoutInfos[i].textures.size() ||
            lhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures.size() != rhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures.size())
        {
            return false;
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutInfos[i].buffers.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].dynamicOffset != rhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].dynamicOffset ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].index != rhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].index ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].stages != rhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].stages ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].type != rhs.layoutInfo.bindGroupLayoutInfos[i].buffers[j].type)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutInfos[i].samplers.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutInfos[i].samplers[j].index != rhs.layoutInfo.bindGroupLayoutInfos[i].samplers[j].index ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].samplers[j].stages != rhs.layoutInfo.bindGroupLayoutInfos[i].samplers[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutInfos[i].textures.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutInfos[i].textures[j].index != rhs.layoutInfo.bindGroupLayoutInfos[i].textures[j].index ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].textures[j].stages != rhs.layoutInfo.bindGroupLayoutInfos[i].textures[j].stages)
            {
                return false;
            }
        }

        for (auto j = 0; j < lhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures.size(); ++j)
        {
            if (lhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].index != rhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].index ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].stages != rhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].stages ||
                lhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].type != rhs.layoutInfo.bindGroupLayoutInfos[i].storageTextures[j].type)
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

    tint::wgsl::reader::Options wgslReaderOptions{
        .allowed_features = tint::wgsl::AllowedFeatures::Everything(),
    };

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
    sprivWriterOptions.bindings = generateBindings(metaData.layoutInfo);
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
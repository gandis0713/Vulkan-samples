#include "vulkan_pipeline.h"
#include "vulkan_device.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_render_pass.h"
#include "vulkan_texture.h"
#include "vulkan_texture_view.h"

#include <array>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace jipu
{

// Vulkan Compute Pipeline
VulkanComputePipeline::VulkanComputePipeline(VulkanDevice& device, const ComputePipelineDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    initialize();
}

VulkanComputePipeline::~VulkanComputePipeline()
{
    auto& vulkanDevice = downcast(m_device);
    vulkanDevice.vkAPI.DestroyPipeline(vulkanDevice.getVkDevice(), m_pipeline, nullptr);
}

PipelineLayout* VulkanComputePipeline::getPipelineLayout() const
{
    return m_descriptor.layout;
}

VkPipeline VulkanComputePipeline::getVkPipeline() const
{
    return m_pipeline;
}

void VulkanComputePipeline::initialize()
{
    auto computeShaderModule = downcast(m_descriptor.compute.shaderModule)->getVkShaderModule();

    VkPipelineShaderStageCreateInfo computeStageInfo{};
    computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStageInfo.module = computeShaderModule;
    computeStageInfo.pName = m_descriptor.compute.entryPoint.c_str();

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.stage = computeStageInfo;
    pipelineCreateInfo.layout = downcast(m_descriptor.layout)->getVkPipelineLayout();
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineCreateInfo.basePipelineIndex = -1;              // Optional

    auto& vulkanDevice = downcast(m_device);
    const VulkanAPI& vkAPI = vulkanDevice.vkAPI;

    if (VK_SUCCESS != vkAPI.CreateComputePipelines(vulkanDevice.getVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline))
    {
        throw std::runtime_error("Failed to create compute pipelines.");
    }
}

// Vulkan Render Pipeline
std::vector<VkVertexInputBindingDescription> generateVertexInputBindingDescription(const RenderPipelineDescriptor& descriptor)
{
    std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
    for (uint64_t bindingIndex = 0; bindingIndex < descriptor.vertex.layouts.size(); ++bindingIndex)
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = static_cast<uint32_t>(bindingIndex);
        bindingDescription.stride = static_cast<uint32_t>(descriptor.vertex.layouts[bindingIndex].stride);
        bindingDescription.inputRate = ToVkVertexInputRate(descriptor.vertex.layouts[bindingIndex].mode);
        bindingDescriptions.push_back(bindingDescription);
    }

    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> generateVertexInputAttributeDescription(const RenderPipelineDescriptor& descriptor)
{
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
    for (uint64_t bindingIndex = 0; bindingIndex < descriptor.vertex.layouts.size(); ++bindingIndex)
    {
        const auto& attributes = descriptor.vertex.layouts[bindingIndex].attributes;
        for (uint64_t attrIndex = 0; attrIndex < attributes.size(); ++attrIndex)
        {
            VkVertexInputAttributeDescription attributeDescription{};
            attributeDescription.binding = attributes[attrIndex].slot;
            attributeDescription.location = attributes[attrIndex].location;
            attributeDescription.format = ToVkVertexFormat(attributes[attrIndex].format);
            attributeDescription.offset = static_cast<uint32_t>(attributes[attrIndex].offset);

            vertexAttributeDescriptions.push_back(attributeDescription);
        }
    }

    return vertexAttributeDescriptions;
}

std::vector<VkPipelineColorBlendAttachmentState> generateColorBlendAttachmentState(const RenderPipelineDescriptor& descriptor)
{
    uint32_t targetSize = static_cast<uint32_t>(descriptor.fragment.targets.size());
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates(targetSize);
    for (auto i = 0; i < targetSize; ++i)
    {
        auto target = descriptor.fragment.targets[i];
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = target.blend.has_value();
        if (colorBlendAttachment.blendEnable)
        {
            auto blend = target.blend.value();
            colorBlendAttachment.srcColorBlendFactor = ToVkBlendFactor(blend.color.srcFactor);
            colorBlendAttachment.dstColorBlendFactor = ToVkBlendFactor(blend.color.dstFactor);
            colorBlendAttachment.colorBlendOp = ToVkBlendOp(blend.color.operation);
            colorBlendAttachment.srcAlphaBlendFactor = ToVkBlendFactor(blend.alpha.srcFactor);
            colorBlendAttachment.dstAlphaBlendFactor = ToVkBlendFactor(blend.alpha.dstFactor);
            colorBlendAttachment.alphaBlendOp = ToVkBlendOp(blend.alpha.operation);
        }
        else
        {
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        colorBlendAttachmentStates[i] = colorBlendAttachment;
    }

    return colorBlendAttachmentStates;
}

VkPipelineInputAssemblyStateCreateInfo generateInputAssemblyStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = ToVkPrimitiveTopology(descriptor.inputAssembly.topology);
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    return inputAssemblyStateCreateInfo;
}

VulkanPipelineVertexInputStateCreateInfo generateVertexInputStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VulkanPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.vertexAttributeDescriptions = generateVertexInputAttributeDescription(descriptor);
    vertexInputStateCreateInfo.vertexBindingDescriptions = generateVertexInputBindingDescription(descriptor);

    return vertexInputStateCreateInfo;
}

VkPipelineViewportStateCreateInfo generateViewportStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = nullptr; // use dynamic state.
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = nullptr; // use dynamic state.

    return viewportStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo generateRasterizationStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = ToVkCullModeFlags(descriptor.rasterization.cullMode);
    rasterizationStateCreateInfo.frontFace = ToVkFrontFace(descriptor.rasterization.frontFace);
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f; // Optional
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;          // Optional
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;    // Optional

    return rasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo generateMultisampleStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = ToVkSampleCountFlagBits(descriptor.rasterization.sampleCount);
    multisampleStateCreateInfo.minSampleShading = 1.0f;          // Optional
    multisampleStateCreateInfo.pSampleMask = nullptr;            // Optional
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;      // Optional

    return multisampleStateCreateInfo;
}

VulkanPipelineColorBlendStateCreateInfo generateColorBlendStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates = generateColorBlendAttachmentState(descriptor);
    VulkanPipelineColorBlendStateCreateInfo colorBlendingStateCreateInfo{};
    colorBlendingStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendingStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlendingStateCreateInfo.attachments = colorBlendAttachmentStates;

    // Used blend constants in dynamic state
    colorBlendingStateCreateInfo.blendConstants[0] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[1] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[2] = 0.0f;
    colorBlendingStateCreateInfo.blendConstants[3] = 0.0f;

    return colorBlendingStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo generateDepthStencilStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = descriptor.depthStencil.has_value();
    depthStencil.depthWriteEnable = descriptor.depthStencil.has_value();
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {};  // Optional

    return depthStencil;
}

VulkanPipelineDynamicStateCreateInfo generateDynamicStateCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        // VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        // VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        // VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };

    VulkanPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.dynamicStates = dynamicStates;

    return dynamicStateCreateInfo;
}

std::vector<VkPipelineShaderStageCreateInfo> generateShaderStageCreateInfo(const RenderPipelineDescriptor& descriptor)
{
    // Vertex Stage
    auto vertexShaderModule = downcast(descriptor.vertex.shaderModule)->getVkShaderModule();

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = vertexShaderModule;
    vertexStageInfo.pName = descriptor.vertex.entryPoint.c_str();

    // Fragment Stage
    auto fragmentShaderModule = downcast(descriptor.fragment.shaderModule)->getVkShaderModule();

    VkPipelineShaderStageCreateInfo fragmentStageInfo{};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragmentShaderModule;
    fragmentStageInfo.pName = descriptor.fragment.entryPoint.c_str();

    return { vertexStageInfo, fragmentStageInfo };
}

VulkanRenderPassDescriptor generateVulkanRenderPassDescriptor(const RenderPipelineDescriptor& descriptor)
{
    if (descriptor.fragment.targets.empty())
        throw std::runtime_error("Failed to create vulkan render pass encoder due to empty fragment target.");

    // Refer to render pass compatibility (https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#renderpass-compatibility)
    VulkanRenderPassDescriptor vkdescriptor{};

    uint32_t targetSize = static_cast<uint32_t>(descriptor.fragment.targets.size());

    for (const auto& target : descriptor.fragment.targets)
    {
        RenderPassColorAttachment renderPassColorAttachment{};

        VkAttachmentDescription renderAttachment{};
        renderAttachment.format = ToVkFormat(target.format);
        renderAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        renderAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        renderAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        renderAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        renderAttachment.samples = ToVkSampleCountFlagBits(descriptor.rasterization.sampleCount);
        renderAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        renderAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        renderPassColorAttachment.renderAttachment = renderAttachment;

        if (descriptor.rasterization.sampleCount > 1)
        {
            VkAttachmentDescription resolveAttachment{};

            resolveAttachment.format = ToVkFormat(target.format);
            resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // should use VK_SAMPLE_COUNT_1_BIT for resolve attachment.
            resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            renderPassColorAttachment.resolveAttachment = resolveAttachment;
        }

        vkdescriptor.colorAttachmentDescriptions.push_back(renderPassColorAttachment);
    }

    if (descriptor.depthStencil.has_value())
    {
        auto depthStencil = descriptor.depthStencil.value();

        VkAttachmentDescription attachment{};
        attachment.format = ToVkFormat(depthStencil.format);
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.samples = ToVkSampleCountFlagBits(descriptor.rasterization.sampleCount);
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vkdescriptor.depthStencilAttachment = attachment;
    }

    {
        VulkanSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // color attachments
        uint32_t colorAttachmentCount = static_cast<uint32_t>(descriptor.fragment.targets.size());
        for (auto i = 0; i < colorAttachmentCount; ++i)
        {
            // attachment references
            VkAttachmentReference colorAttachmentReference{};
            colorAttachmentReference.attachment = i;
            colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpassDescription.colorAttachments.push_back(colorAttachmentReference);
        }

        std::vector<VkAttachmentReference> resolveAttachmentReferences{};
        if (descriptor.rasterization.sampleCount > 1)
        {
            // resolve attachments
            for (uint32_t i = colorAttachmentCount; i < colorAttachmentCount * 2; ++i)
            {
                VkAttachmentReference resolveAttachmentReference{};
                resolveAttachmentReference.attachment = i;
                resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                subpassDescription.resolveAttachments.push_back(resolveAttachmentReference);
            }
        }

        if (descriptor.depthStencil.has_value())
        {
            VkAttachmentReference depthAttachment{};
            depthAttachment.attachment = static_cast<uint32_t>(subpassDescription.colorAttachments.size() + subpassDescription.resolveAttachments.size());
            depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpassDescription.depthStencilAttachment = depthAttachment;
        }
        vkdescriptor.subpassDescriptions = { subpassDescription };

        VkSubpassDependency subpassDependency{};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        subpassDependency.dstSubpass = 0;
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask = 0;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (descriptor.depthStencil.has_value())
            subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        vkdescriptor.subpassDependencies = { subpassDependency };
    }

    return vkdescriptor;
}

VulkanRenderPipelineDescriptor generateVulkanRenderPipelineDescriptor(VulkanDevice& device, const RenderPipelineDescriptor& descriptor)
{
    VulkanRenderPipelineDescriptor vkdescriptor{
        .next = nullptr,
        .flags = 0u,
        .stages = generateShaderStageCreateInfo(descriptor),
        .inputAssemblyState = generateInputAssemblyStateCreateInfo(descriptor),
        .vertexInputState = generateVertexInputStateCreateInfo(descriptor),
        .viewportState = generateViewportStateCreateInfo(descriptor),
        .rasterizationState = generateRasterizationStateCreateInfo(descriptor),
        .multisampleState = generateMultisampleStateCreateInfo(descriptor),
        .depthStencilState = generateDepthStencilStateCreateInfo(descriptor),
        .colorBlendState = generateColorBlendStateCreateInfo(descriptor),
        .dynamicState = generateDynamicStateCreateInfo(descriptor),
        .layout = downcast(descriptor.layout),
        .renderPass = device.getRenderPass(generateVulkanRenderPassDescriptor(descriptor)),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE, // Optional
        .basePipelineIndex = -1,              // Optional
    };

    return vkdescriptor;
}

VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice& device, const RenderPipelineDescriptor& descriptor)
    : VulkanRenderPipeline(device, generateVulkanRenderPipelineDescriptor(device, descriptor))
{
}

VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice& device, const VulkanRenderPipelineDescriptor& descriptor)
    : m_device(device)
    , m_descriptor(descriptor)
{
    initialize();
}

VulkanRenderPipeline::~VulkanRenderPipeline()
{
    auto& vulkanDevice = downcast(m_device);
    vulkanDevice.vkAPI.DestroyPipeline(vulkanDevice.getVkDevice(), m_pipeline, nullptr);
}

PipelineLayout* VulkanRenderPipeline::getPipelineLayout() const
{
    return m_descriptor.layout;
}

VkPipeline VulkanRenderPipeline::getVkPipeline() const
{
    return m_pipeline;
}

void VulkanRenderPipeline::initialize()
{
    const auto& descriptor = m_descriptor;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(descriptor.vertexInputState.vertexBindingDescriptions.size());
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(descriptor.vertexInputState.vertexAttributeDescriptions.size());
    vertexInputStateCreateInfo.pVertexBindingDescriptions = descriptor.vertexInputState.vertexBindingDescriptions.data();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = descriptor.vertexInputState.vertexAttributeDescriptions.data();

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo{};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.attachmentCount = static_cast<uint32_t>(descriptor.colorBlendState.attachments.size());
    colorBlendCreateInfo.pAttachments = descriptor.colorBlendState.attachments.data();
    colorBlendCreateInfo.logicOpEnable = descriptor.colorBlendState.logicOpEnable;
    colorBlendCreateInfo.logicOp = descriptor.colorBlendState.logicOp;
    colorBlendCreateInfo.blendConstants[0] = descriptor.colorBlendState.blendConstants[0];
    colorBlendCreateInfo.blendConstants[1] = descriptor.colorBlendState.blendConstants[1];
    colorBlendCreateInfo.blendConstants[2] = descriptor.colorBlendState.blendConstants[2];
    colorBlendCreateInfo.blendConstants[3] = descriptor.colorBlendState.blendConstants[3];

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(descriptor.dynamicState.dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = descriptor.dynamicState.dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(descriptor.stages.size());
    pipelineInfo.pStages = descriptor.stages.data();
    pipelineInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineInfo.pInputAssemblyState = &descriptor.inputAssemblyState;
    pipelineInfo.pViewportState = &descriptor.viewportState;
    pipelineInfo.pRasterizationState = &descriptor.rasterizationState;
    pipelineInfo.pMultisampleState = &descriptor.multisampleState;
    pipelineInfo.pDepthStencilState = &descriptor.depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.layout = downcast(descriptor.layout)->getVkPipelineLayout();
    pipelineInfo.renderPass = descriptor.renderPass->getVkRenderPass();
    pipelineInfo.subpass = descriptor.subpass;
    pipelineInfo.basePipelineHandle = descriptor.basePipelineHandle;
    pipelineInfo.basePipelineIndex = descriptor.basePipelineIndex;

    auto& vulkanDevice = downcast(m_device);
    if (vulkanDevice.vkAPI.CreateGraphicsPipelines(vulkanDevice.getVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
}

VkFormat ToVkVertexFormat(VertexFormat format)
{
    switch (format)
    {
    case VertexFormat::kUint8x2:
        return VK_FORMAT_R8G8_UINT;
    case VertexFormat::kUint8x4:
        return VK_FORMAT_R8G8B8A8_UINT;
    case VertexFormat::kSint8x2:
        return VK_FORMAT_R8G8_SINT;
    case VertexFormat::kSint8x4:
        return VK_FORMAT_R8G8B8A8_SINT;
    case VertexFormat::kUnorm8x2:
        return VK_FORMAT_R8G8_UNORM;
    case VertexFormat::kUnorm8x4:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case VertexFormat::kSnorm8x2:
        return VK_FORMAT_R8G8_SNORM;
    case VertexFormat::kSnorm8x4:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case VertexFormat::kUint16x2:
        return VK_FORMAT_R16G16_UINT;
    case VertexFormat::kUint16x4:
        return VK_FORMAT_R16G16B16A16_UINT;
    case VertexFormat::kSint16x2:
        return VK_FORMAT_R16G16_SINT;
    case VertexFormat::kSint16x4:
        return VK_FORMAT_R16G16B16A16_SINT;
    case VertexFormat::kUnorm16x2:
        return VK_FORMAT_R16G16_UNORM;
    case VertexFormat::kUnorm16x4:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case VertexFormat::kSnorm16x2:
        return VK_FORMAT_R16G16_SNORM;
    case VertexFormat::kSnorm16x4:
        return VK_FORMAT_R16G16B16A16_SNORM;
    case VertexFormat::kFloat16x2:
        return VK_FORMAT_R16G16_SFLOAT;
    case VertexFormat::kFloat16x4:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case VertexFormat::kFloat32:
        return VK_FORMAT_R32_SFLOAT;
    case VertexFormat::kFloat32x2:
        return VK_FORMAT_R32G32_SFLOAT;
    case VertexFormat::kFloat32x3:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case VertexFormat::kFloat32x4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case VertexFormat::kUint32:
        return VK_FORMAT_R32_UINT;
    case VertexFormat::kUint32x2:
        return VK_FORMAT_R32G32_UINT;
    case VertexFormat::kUint32x3:
        return VK_FORMAT_R32G32B32_UINT;
    case VertexFormat::kUint32x4:
        return VK_FORMAT_R32G32B32A32_UINT;
    case VertexFormat::kSint32:
        return VK_FORMAT_R32_SINT;
    case VertexFormat::kSint32x2:
        return VK_FORMAT_R32G32_SINT;
    case VertexFormat::kSint32x3:
        return VK_FORMAT_R32G32B32_SINT;
    case VertexFormat::kSint32x4:
        return VK_FORMAT_R32G32B32A32_SINT;
    case VertexFormat::kUnorm10_10_10_2:
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    default:
        spdlog::error("{} vertex format is not supported.", static_cast<uint32_t>(format));
        return VK_FORMAT_UNDEFINED;
    }
}

VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
    case PrimitiveTopology::kPointList:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    case PrimitiveTopology::kLineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;

    case PrimitiveTopology::kLineList:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    case PrimitiveTopology::kTriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    case PrimitiveTopology::kTriangleList:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    default:
        spdlog::error("{} topology for primitive is not supported.", static_cast<uint32_t>(topology));
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
}

VkCullModeFlags ToVkCullModeFlags(CullMode mode)
{
    VkCullModeFlags flags = 0u;
    switch (mode)
    {
    case CullMode::kFront:
        flags = VK_CULL_MODE_FRONT_BIT;
        break;
    case CullMode::kBack:
        flags = VK_CULL_MODE_BACK_BIT;
        break;
    case CullMode::kNone:
    default:
        flags = VK_CULL_MODE_NONE;
        break;
    }

    return flags;
}

VkFrontFace ToVkFrontFace(FrontFace frontFace)
{
    VkFrontFace face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    switch (frontFace)
    {
    case FrontFace::kClockwise:
        face = VK_FRONT_FACE_CLOCKWISE;
        break;
    case FrontFace::kCounterClockwise:
    default:
        face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        break;
    }

    return face;
}

VkBlendOp ToVkBlendOp(BlendOperation op)
{
    VkBlendOp blendOp = VK_BLEND_OP_ADD;

    switch (op)
    {
    default:
    case BlendOperation::kAdd:
        blendOp = VK_BLEND_OP_ADD;
        break;
    case BlendOperation::kSubtract:
        blendOp = VK_BLEND_OP_SUBTRACT;
        break;
    case BlendOperation::kReversSubtract:
        blendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
        break;
    case BlendOperation::kMin:
        blendOp = VK_BLEND_OP_MIN;
        break;
    case BlendOperation::kMax:
        blendOp = VK_BLEND_OP_MAX;
        break;
    }

    return blendOp;
}

VkBlendFactor ToVkBlendFactor(BlendFactor factor)
{
    VkBlendFactor blendFactor = VK_BLEND_FACTOR_ONE;

    switch (factor)
    {
    default:
    case BlendFactor::kZero:
        blendFactor = VK_BLEND_FACTOR_ZERO;
        break;
    case BlendFactor::kOne:
        blendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case BlendFactor::kSrcColor:
        blendFactor = VK_BLEND_FACTOR_SRC_COLOR;
        break;
    case BlendFactor::kSrcAlpha:
        blendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        break;
    case BlendFactor::kSrcAlphaSaturated:
        blendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        break;
    case BlendFactor::kOneMinusSrcColor:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        break;
    case BlendFactor::kOneMinusSrcAlpha:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BlendFactor::kDstColor:
        blendFactor = VK_BLEND_FACTOR_DST_COLOR;
        break;
    case BlendFactor::kDstAlpha:
        blendFactor = VK_BLEND_FACTOR_DST_ALPHA;
        break;
    case BlendFactor::kOneMinusDstColor:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        break;
    case BlendFactor::kOneMinusDstAlpha:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        break;
    case BlendFactor::kConstantColor:
        blendFactor = VK_BLEND_FACTOR_CONSTANT_COLOR;
        break;
    case BlendFactor::kOneMinusConstantColor:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        break;
    case BlendFactor::kSrc1Color:
        blendFactor = VK_BLEND_FACTOR_SRC1_COLOR;
        break;
    case BlendFactor::kSrc1Alpha:
        blendFactor = VK_BLEND_FACTOR_SRC1_ALPHA;
        break;
    case BlendFactor::kOneMinusSrc1Color:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        break;
    case BlendFactor::kOneMinusSrc1Alpha:
        blendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        break;
    }

    return blendFactor;
}

VkVertexInputRate ToVkVertexInputRate(VertexMode mode)
{
    VkVertexInputRate inputRate;

    switch (mode)
    {
    case VertexMode::kVertex:
    default:
        inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        break;
    case VertexMode::kInstance:
        inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        break;
    }

    return inputRate;
}

} // namespace jipu

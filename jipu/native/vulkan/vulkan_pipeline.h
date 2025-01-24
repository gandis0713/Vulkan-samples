#pragma once

#include "jipu/common/cast.h"
#include "pipeline.h"
#include "vulkan_api.h"
#include "vulkan_export.h"
#include "vulkan_pipeline_layout.h"
#include "vulkan_render_pass.h"
#include "vulkan_shader_module.h"

#include <string>
#include <vector>

namespace jipu
{

class VulkanDevice;
class VulkanPipelineLayout;
class VULKAN_EXPORT VulkanComputePipeline : public ComputePipeline
{
public:
    VulkanComputePipeline() = delete;
    VulkanComputePipeline(VulkanDevice* device, const ComputePipelineDescriptor& descriptor);
    ~VulkanComputePipeline() override;

    VulkanComputePipeline(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;

public:
    VkPipelineLayout getVkPipelineLayout() const;

public:
    VkPipeline getVkPipeline() const;
    VkShaderModule getShaderModule() const;

private:
    void initialize();

private:
    VulkanDevice* m_device = nullptr;
    const ComputePipelineDescriptor m_descriptor;
    VulkanPipelineLayoutInfo m_layoutInfo{};

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
DOWN_CAST(VulkanComputePipeline, ComputePipeline);

// Vulkan Render Pipeline

struct VulkanPipelineVertexInputStateCreateInfo
{
    const void* next = nullptr;
    VkPipelineVertexInputStateCreateFlags flags = 0u;
    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions{};
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions{};
};

struct VulkanPipelineColorBlendStateCreateInfo
{
    const void* next = nullptr;
    VkPipelineColorBlendStateCreateFlags flags = 0u;
    VkBool32 logicOpEnable;
    VkLogicOp logicOp = VK_LOGIC_OP_CLEAR;
    std::vector<VkPipelineColorBlendAttachmentState> attachments{};
    float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct VulkanPipelineDynamicStateCreateInfo
{
    const void* next = nullptr;
    VkPipelineDynamicStateCreateFlags flags = 0u;
    std::vector<VkDynamicState> dynamicStates{};
};

struct VulkanRenderPipelineDescriptor
{
    const void* next = nullptr;
    VkPipelineCreateFlags flags = 0u;
    std::vector<VkPipelineShaderStageCreateInfo> stages{};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    VulkanPipelineVertexInputStateCreateInfo vertexInputState{};
    VkPipelineTessellationStateCreateInfo tessellationState{};
    VkPipelineViewportStateCreateInfo viewportState{};
    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    VkPipelineMultisampleStateCreateInfo multisampleState{};
    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    VulkanPipelineColorBlendStateCreateInfo colorBlendState{};
    VulkanPipelineDynamicStateCreateInfo dynamicState{};
    VulkanPipelineLayout* layout = nullptr;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
    VkPipeline basePipelineHandle = VK_NULL_HANDLE;
    int32_t basePipelineIndex = -1;
};

class VulkanRenderPass;
class VULKAN_EXPORT VulkanRenderPipeline : public RenderPipeline
{
public:
    VulkanRenderPipeline() = delete;
    VulkanRenderPipeline(VulkanDevice* device, const RenderPipelineDescriptor& descriptor);
    VulkanRenderPipeline(VulkanDevice* device, const VulkanRenderPipelineDescriptor& descriptor);
    ~VulkanRenderPipeline() override;

    VulkanRenderPipeline(const VulkanRenderPipeline&) = delete;
    VulkanRenderPipeline& operator=(const VulkanRenderPipeline&) = delete;

public:
    VkPipelineLayout getVkPipelineLayout() const;

public:
    VkPipeline getVkPipeline() const;
    std::vector<VkShaderModule> getShaderModules() const;

private:
    void initialize();

private:
    VulkanDevice* m_device = nullptr;
    const VulkanRenderPipelineDescriptor m_descriptor;
    VulkanPipelineLayoutInfo m_layoutInfo{};

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};
DOWN_CAST(VulkanRenderPipeline, RenderPipeline);

// Generate Helper
std::vector<VkVertexInputBindingDescription> VULKAN_EXPORT generateVertexInputBindingDescription(const RenderPipelineDescriptor& descriptor);
std::vector<VkVertexInputAttributeDescription> VULKAN_EXPORT generateVertexInputAttributeDescription(const RenderPipelineDescriptor& descriptor);
std::vector<VkPipelineColorBlendAttachmentState> VULKAN_EXPORT generateColorBlendAttachmentState(const RenderPipelineDescriptor& descriptor);
VkPipelineInputAssemblyStateCreateInfo VULKAN_EXPORT generateInputAssemblyStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VulkanPipelineVertexInputStateCreateInfo VULKAN_EXPORT generateVertexInputStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VkPipelineViewportStateCreateInfo VULKAN_EXPORT generateViewportStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VkPipelineRasterizationStateCreateInfo VULKAN_EXPORT generateRasterizationStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VkPipelineMultisampleStateCreateInfo VULKAN_EXPORT generateMultisampleStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VulkanPipelineColorBlendStateCreateInfo VULKAN_EXPORT generateColorBlendStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VkPipelineDepthStencilStateCreateInfo VULKAN_EXPORT generateDepthStencilStateCreateInfo(const RenderPipelineDescriptor& descriptor);
VulkanPipelineDynamicStateCreateInfo VULKAN_EXPORT generateDynamicStateCreateInfo(const RenderPipelineDescriptor& descriptor);
std::vector<VkPipelineShaderStageCreateInfo> VULKAN_EXPORT generateShaderStageCreateInfo(const RenderPipelineDescriptor& descriptor);
VulkanRenderPipelineDescriptor VULKAN_EXPORT generateVulkanRenderPipelineDescriptor(VulkanDevice* device, const RenderPipelineDescriptor& descriptor);

// Convert Helper
VkFormat ToVkVertexFormat(VertexFormat format);
VkPrimitiveTopology ToVkPrimitiveTopology(PrimitiveTopology topology);
VkCullModeFlags ToVkCullModeFlags(CullMode mode);
VkFrontFace ToVkFrontFace(FrontFace frontFace);
VkBlendOp ToVkBlendOp(BlendOperation op);
VkBlendFactor ToVkBlendFactor(BlendFactor factor);
VkVertexInputRate ToVkVertexInputRate(VertexMode mode);
VkCompareOp ToVkCompareOp(CompareFunction compareFunction);

} // namespace jipu

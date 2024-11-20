

#include "vulkan_pipeline_barrier_sample.h"

#include "vulkan_command_encoder.h"
#include "vulkan_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_render_pass_encoder.h"
#include "vulkan_texture.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include <set>

namespace jipu
{

namespace
{

template <typename T>
inline size_t hash(const T& value)
{
    return std::hash<T>()(value);
}

template <class T>
inline void combineHash(size_t& seed, const T& value) noexcept
{
    seed ^= std::hash<T>()(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
}

std::unordered_map<VkPipelineStageFlags, const char*> STAGE_STRINGS{
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, "TOP_OF_PIPE_BIT" },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, "DRAW_INDIRECT_BIT" },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, "VERTEX_INPUT_BIT" },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, "VERTEX_SHADER_BIT" },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, "TESSELLATION_CONTROL_SHADER_BIT" },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, "TESSELLATION_EVALUATION_SHADER_BIT" },
    { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, "GEOMETRY_SHADER_BIT" },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, "FRAGMENT_SHADER_BIT" },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, "EARLY_FRAGMENT_TESTS_BIT" },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, "LATE_FRAGMENT_TESTS_BIT" },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, "COLOR_ATTACHMENT_OUTPUT_BIT" },
    { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, "COMPUTE_SHADER_BIT" },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, "TRANSFER_BIT" },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, "BOTTOM_OF_PIPE_BIT" },
};

std::vector<Stage> STAGES{
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },

    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    // { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
    { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_NONE },
};

} // namespace

size_t StageHash::operator()(const Stage& stage) const
{
    size_t v = hash(stage.srcStageMask);

    combineHash(v, stage.dstStageMask);

    return v;
}

bool StageEqual::operator()(const Stage& lhs, const Stage& rhs) const
{
    if (lhs.srcStageMask == rhs.srcStageMask &&
        lhs.dstStageMask == rhs.dstStageMask)
        return true;

    return false;
}

VulkanPipelineBarrierSample::VulkanPipelineBarrierSample(const SampleDescriptor& descriptor)
    : Sample(descriptor)
{
}

VulkanPipelineBarrierSample::~VulkanPipelineBarrierSample()
{
    m_offscreen.renderPipelines.clear();
    m_offscreen.renderPipelineLayout.reset();
    m_offscreen.bindGroup.reset();
    m_offscreen.bindGroupLayout.reset();
    m_offscreen.vertexBuffer.reset();
    m_offscreen.indexBuffer.reset();
    m_offscreen.uniformBuffer.reset();
    m_offscreen.renderTextureView.reset();
    m_offscreen.renderTexture.reset();

    m_onscreen.renderPipeline.reset();
    m_onscreen.renderPipelineLayout.reset();
    m_onscreen.bindGroup.reset();
    m_onscreen.bindGroupLayout.reset();
    m_onscreen.vertexBuffer.reset();
    m_onscreen.indexBuffer.reset();
    m_onscreen.sampler.reset();
}

void VulkanPipelineBarrierSample::init()
{
    Sample::init();

    createHPCWatcher();

    createOffscreenTexture();
    createOffscreenTextureView();
    createOffscreenVertexBuffer();
    createOffscreenIndexBuffer();
    createOffscreenUniformBuffer();
    createOffscreenBindGroupLayout();
    createOffscreenBindGroup();
    createOffscreenRenderPipeline();

    createOnscreenVertexBuffer();
    createOnscreenIndexBuffer();
    createOnscreenSampler();
    createOnscreenBindGroupLayout();
    createOnscreenBindGroup();
    createOnscreenRenderPipeline();

    createCamera();
}

void VulkanPipelineBarrierSample::createCamera()
{
    m_camera = std::make_unique<PerspectiveCamera>(45.0f,
                                                   m_width / static_cast<float>(m_height),
                                                   0.1f,
                                                   1000.0f);

    // auto halfWidth = m_width / 2.0f;
    // auto halfHeight = m_height / 2.0f;
    // m_camera = std::make_unique<OrthographicCamera>(-halfWidth, halfWidth,
    //                                                 -halfHeight, halfHeight,
    //                                                 -1000, 1000);

    m_camera->lookAt(glm::vec3(0.0f, 0.0f, 1000.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0f, 0.0));
}

void VulkanPipelineBarrierSample::updateOffscreenUniformBuffer()
{
    m_ubo.mvp.model = glm::mat4(1.0f);
    m_ubo.mvp.view = m_camera->getViewMat();
    m_ubo.mvp.proj = m_camera->getProjectionMat();

    void* pointer = m_offscreen.uniformBuffer->map(); // do not unmap.
    memcpy(pointer, &m_ubo, m_offscreen.uniformBuffer->getSize());
}

void VulkanPipelineBarrierSample::onUpdate()
{
    Sample::onUpdate();

    updateOffscreenUniformBuffer();

    updateImGui();
}

void VulkanPipelineBarrierSample::onDraw()
{
    auto renderView = m_swapchain->acquireNextTextureView();

    // offscreen pass
    {
        VulkanRenderPassEncoderDescriptor renderPassEncoderDescriptor{};
        renderPassEncoderDescriptor.renderPass = getOffscreenRenderPass(m_stage).getVkRenderPass();
        renderPassEncoderDescriptor.framebuffer = getOffscreenFramebuffer(*m_offscreen.renderTextureView).getVkFrameBuffer();
        renderPassEncoderDescriptor.renderArea.offset = { 0, 0 };
        renderPassEncoderDescriptor.renderArea.extent = { m_swapchain->getWidth(), m_swapchain->getHeight() };

        VkClearValue colorClearValue{};
        colorClearValue.color.float32[0] = 0.0f;
        colorClearValue.color.float32[1] = 0.0f;
        colorClearValue.color.float32[2] = 0.0f;
        colorClearValue.color.float32[3] = 0.0f;
        renderPassEncoderDescriptor.clearValues.push_back(colorClearValue);

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_commandBuffer->createCommandEncoder(commandDescriptor);
        auto vulkanCommandEncoder = downcast(commandEncoder.get());

        auto renderPassEncoder = vulkanCommandEncoder->beginRenderPass(renderPassEncoderDescriptor);
        renderPassEncoder->setPipeline(m_offscreen.renderPipelines[m_stage].get());
        renderPassEncoder->setBindGroup(0, *m_offscreen.bindGroup);
        renderPassEncoder->setVertexBuffer(0, *m_offscreen.vertexBuffer);
        renderPassEncoder->setIndexBuffer(*m_offscreen.indexBuffer, IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_offscreenIndices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        m_queue->submit({ commandEncoder->finish() });
    }

    // onscreen pass
    {
        ColorAttachment attachment{
            .renderView = renderView
        };
        attachment.clearValue = { 0.0, 0.0, 0.0, 0.0 };
        attachment.loadOp = LoadOp::kClear;
        attachment.storeOp = StoreOp::kStore;

        RenderPassEncoderDescriptor renderPassDescriptor{
            .colorAttachments = { attachment }
        };

        CommandEncoderDescriptor commandDescriptor{};
        auto commandEncoder = m_device->createCommandEncoder(commandDescriptor);

        auto renderPassEncoder = commandEncoder->beginRenderPass(renderPassDescriptor);
        renderPassEncoder->setPipeline(m_onscreen.renderPipeline.get());
        renderPassEncoder->setBindGroup(0, *m_onscreen.bindGroup);
        renderPassEncoder->setVertexBuffer(0, *m_onscreen.vertexBuffer);
        renderPassEncoder->setIndexBuffer(*m_onscreen.indexBuffer, IndexFormat::kUint16);
        renderPassEncoder->setScissor(0, 0, m_width, m_height);
        renderPassEncoder->setViewport(0, 0, m_width, m_height, 0, 1);
        renderPassEncoder->drawIndexed(static_cast<uint32_t>(m_onscreenIndices.size()), 1, 0, 0, 0);
        renderPassEncoder->end();

        drawImGui(commandEncoder.get(), renderView);

        m_queue->submit({ commandEncoder->finish() }, *m_swapchain);
    }
}

void VulkanPipelineBarrierSample::updateImGui()
{

    recordImGui({ [&]() {
        windowImGui("Source Stage", { [&]() {
                        std::set<VkPipelineStageFlags> srcStageMasks{};
                        for (auto stage : STAGES)
                        {
                            srcStageMasks.insert(stage.srcStageMask);
                        }

                        for (auto srcStageMask : srcStageMasks)
                        {
                            if (ImGui::RadioButton(STAGE_STRINGS[srcStageMask], m_stage.srcStageMask == srcStageMask))
                                m_stage.srcStageMask = srcStageMask;
                        }
                    } });
        windowImGui("Destination Stage", { [&]() {
                        for (auto stage : STAGES)
                        {
                            if (stage.srcStageMask == m_stage.srcStageMask)
                            {
                                if (ImGui::RadioButton(STAGE_STRINGS[stage.dstStageMask], m_stage.dstStageMask == stage.dstStageMask))
                                    m_stage.dstStageMask = stage.dstStageMask;
                            }
                        }
                    } });
        profilingWindow();
    } });
}

void VulkanPipelineBarrierSample::createOffscreenTexture()
{
#if defined(__ANDROID__) || defined(ANDROID)
    TextureFormat textureFormat = TextureFormat::kRGBA8Unorm;
#else
    TextureFormat textureFormat = TextureFormat::kBGRA8Unorm;
#endif

    TextureDescriptor textureDescriptor;
    textureDescriptor.width = m_width;
    textureDescriptor.height = m_height;
    textureDescriptor.depth = 1;
    textureDescriptor.format = textureFormat;
    textureDescriptor.usage = TextureUsageFlagBits::kColorAttachment | TextureUsageFlagBits::kTextureBinding;
    textureDescriptor.type = TextureType::k2D;
    textureDescriptor.sampleCount = 1; // TODO: set from descriptor
    textureDescriptor.mipLevels = 1;   // TODO: set from descriptor

    m_offscreen.renderTexture = m_device->createTexture(textureDescriptor);
}

void VulkanPipelineBarrierSample::createOffscreenTextureView()
{
    TextureViewDescriptor textureViewDescriptor;
    textureViewDescriptor.aspect = TextureAspectFlagBits::kColor;
    textureViewDescriptor.dimension = TextureViewDimension::k2D;

    m_offscreen.renderTextureView = m_offscreen.renderTexture->createTextureView(textureViewDescriptor);
}

void VulkanPipelineBarrierSample::createOffscreenVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_offscreenVertices.size() * sizeof(OffscreenVertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_offscreen.vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.vertexBuffer->map();
    memcpy(pointer, m_offscreenVertices.data(), descriptor.size);
    m_offscreen.vertexBuffer->unmap();
}

void VulkanPipelineBarrierSample::createOffscreenIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_offscreenIndices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_offscreen.indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.indexBuffer->map();
    memcpy(pointer, m_offscreenIndices.data(), descriptor.size);
    m_offscreen.indexBuffer->unmap();
}

void VulkanPipelineBarrierSample::createOffscreenUniformBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = sizeof(UBO);
    descriptor.usage = BufferUsageFlagBits::kUniform;

    m_offscreen.uniformBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_offscreen.uniformBuffer->map();
    // memcpy(pointer, &m_ubo, descriptor.size);
    // m_offscreen.uniformBuffer->unmap();
}

void VulkanPipelineBarrierSample::createOffscreenBindGroupLayout()
{
    BufferBindingLayout bufferLayout{};
    bufferLayout.index = 0;
    bufferLayout.stages = BindingStageFlagBits::kVertexStage;
    bufferLayout.type = BufferBindingType::kUniform;

    BindGroupLayoutDescriptor descriptor{};
    descriptor.buffers = { bufferLayout };

    m_offscreen.bindGroupLayout = m_device->createBindGroupLayout(descriptor);
}

void VulkanPipelineBarrierSample::createOffscreenBindGroup()
{
    BufferBinding bufferBinding{
        .index = 0,
        .offset = 0,
        .size = m_offscreen.uniformBuffer->getSize(),
        .buffer = m_offscreen.uniformBuffer.get(),
    };

    BindGroupDescriptor descriptor{
        .layout = m_offscreen.bindGroupLayout.get(),
        .buffers = { bufferBinding },
    };

    m_offscreen.bindGroup = m_device->createBindGroup(descriptor);
}

void VulkanPipelineBarrierSample::createOffscreenRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_offscreen.bindGroupLayout.get() };

        m_offscreen.renderPipelineLayout = m_device->createPipelineLayout(descriptor);
    }

    // input assembly stage
    InputAssemblyStage inputAssemblyStage{};
    {
        inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex shader module
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "offscreen.vert.spv", m_handle);
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage

    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(OffscreenVertex, pos);
    positionAttribute.location = 0;

    VertexAttribute colorAttribute{};
    colorAttribute.format = VertexFormat::kFloat32x3;
    colorAttribute.offset = offsetof(OffscreenVertex, color);
    colorAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(OffscreenVertex);
    vertexInputLayout.attributes = { positionAttribute, colorAttribute };

    VertexStage vertexStage{
        { vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // rasterization
    RasterizationStage rasterizationStage{};
    {
        rasterizationStage.cullMode = CullMode::kNone;
        rasterizationStage.frontFace = FrontFace::kCounterClockwise;
        rasterizationStage.sampleCount = m_sampleCount;
    }

    // fragment shader module
    std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "offscreen.frag.spv", m_handle);
        descriptor.code = fragmentShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(fragmentShaderSource.size());

        fragmentShaderModule = m_device->createShaderModule(descriptor);
    }

    // fragment

    FragmentStage::Target target{};
    target.format = m_offscreen.renderTexture->getFormat();

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" },
        { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        m_offscreen.renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    VulkanRenderPipelineDescriptor vkdescriptor = generateVulkanRenderPipelineDescriptor(downcast(*m_device), descriptor);

    for (auto stage : STAGES)
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
            .renderPass = getOffscreenRenderPass(stage),
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE, // Optional
            .basePipelineIndex = -1,              // Optional
        };

        m_offscreen.renderPipelines[stage] = downcast(m_device.get())->createRenderPipeline(vkdescriptor);
    }
}

void VulkanPipelineBarrierSample::createOnscreenVertexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_onscreenVertices.size() * sizeof(OnscreenVertex);
    descriptor.usage = BufferUsageFlagBits::kVertex;

    m_onscreen.vertexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_onscreen.vertexBuffer->map();
    memcpy(pointer, m_onscreenVertices.data(), descriptor.size);
    m_onscreen.vertexBuffer->unmap();
}

void VulkanPipelineBarrierSample::createOnscreenIndexBuffer()
{
    BufferDescriptor descriptor{};
    descriptor.size = m_onscreenIndices.size() * sizeof(uint16_t);
    descriptor.usage = BufferUsageFlagBits::kIndex;

    m_onscreen.indexBuffer = m_device->createBuffer(descriptor);

    void* pointer = m_onscreen.indexBuffer->map();
    memcpy(pointer, m_onscreenIndices.data(), descriptor.size);
    m_onscreen.indexBuffer->unmap();
}

void VulkanPipelineBarrierSample::createOnscreenSampler()
{
    SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeV = AddressMode::kClampToEdge;
    samplerDescriptor.addressModeW = AddressMode::kClampToEdge;
    samplerDescriptor.lodMin = 0.0f;
    samplerDescriptor.lodMax = static_cast<float>(m_offscreen.renderTexture->getMipLevels());
    samplerDescriptor.minFilter = FilterMode::kLinear;
    samplerDescriptor.magFilter = FilterMode::kLinear;
    samplerDescriptor.mipmapFilter = MipmapFilterMode::kLinear;

    m_onscreen.sampler = m_device->createSampler(samplerDescriptor);
}

void VulkanPipelineBarrierSample::createOnscreenBindGroupLayout()
{
    SamplerBindingLayout samplerLayout{};
    samplerLayout.index = 0;
    samplerLayout.stages = BindingStageFlagBits::kFragmentStage;

    TextureBindingLayout textureLayout{};
    textureLayout.index = 1;
    textureLayout.stages = BindingStageFlagBits::kFragmentStage;

    BindGroupLayoutDescriptor descriptor{};
    descriptor.samplers = { samplerLayout };
    descriptor.textures = { textureLayout };

    m_onscreen.bindGroupLayout = m_device->createBindGroupLayout(descriptor);
}

void VulkanPipelineBarrierSample::createOnscreenBindGroup()
{
    SamplerBinding samplerBinding{
        .index = 0,
        .sampler = m_onscreen.sampler.get(),
    };

    TextureBinding textureBinding{
        .index = 1,
        .textureView = m_offscreen.renderTextureView.get(),
    };

    BindGroupDescriptor descriptor{
        .layout = m_onscreen.bindGroupLayout.get(),
        .samplers = { samplerBinding },
        .textures = { textureBinding },
    };

    m_onscreen.bindGroup = m_device->createBindGroup(descriptor);
}

void VulkanPipelineBarrierSample::createOnscreenRenderPipeline()
{
    // render pipeline layout
    {
        PipelineLayoutDescriptor descriptor{};
        descriptor.layouts = { m_onscreen.bindGroupLayout.get() };

        m_onscreen.renderPipelineLayout = m_device->createPipelineLayout(descriptor);
    }

    // input assembly stage
    InputAssemblyStage inputAssemblyStage{};
    {
        inputAssemblyStage.topology = PrimitiveTopology::kTriangleList;
    }

    // vertex shader module
    std::unique_ptr<ShaderModule> vertexShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> vertexShaderSource = utils::readFile(m_appDir / "onscreen.vert.spv", m_handle);
        descriptor.code = vertexShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(vertexShaderSource.size());

        vertexShaderModule = m_device->createShaderModule(descriptor);
    }

    // vertex stage
    VertexAttribute positionAttribute{};
    positionAttribute.format = VertexFormat::kFloat32x3;
    positionAttribute.offset = offsetof(OnscreenVertex, pos);
    positionAttribute.location = 0;

    VertexAttribute texCoordAttribute{};
    texCoordAttribute.format = VertexFormat::kFloat32x2;
    texCoordAttribute.offset = offsetof(OnscreenVertex, texCoord);
    texCoordAttribute.location = 1;

    VertexInputLayout vertexInputLayout{};
    vertexInputLayout.mode = VertexMode::kVertex;
    vertexInputLayout.stride = sizeof(OnscreenVertex);
    vertexInputLayout.attributes = { positionAttribute, texCoordAttribute };

    VertexStage vertexStage{
        { vertexShaderModule.get(), "main" },
        { vertexInputLayout }
    };

    // rasterization
    RasterizationStage rasterizationStage{};
    {
        rasterizationStage.cullMode = CullMode::kNone;
        rasterizationStage.frontFace = FrontFace::kCounterClockwise;
        rasterizationStage.sampleCount = m_sampleCount;
    }

    // fragment shader module
    std::unique_ptr<ShaderModule> fragmentShaderModule = nullptr;
    {
        ShaderModuleDescriptor descriptor{};
        std::vector<char> fragmentShaderSource = utils::readFile(m_appDir / "onscreen.frag.spv", m_handle);
        descriptor.code = fragmentShaderSource.data();
        descriptor.codeSize = static_cast<uint32_t>(fragmentShaderSource.size());

        fragmentShaderModule = m_device->createShaderModule(descriptor);
    }

    // fragment
    FragmentStage::Target target{};
    target.format = m_swapchain->getTextureFormat();

    FragmentStage fragmentStage{
        { fragmentShaderModule.get(), "main" },
        { target }
    };

    // depth/stencil

    // render pipeline
    RenderPipelineDescriptor descriptor{
        m_onscreen.renderPipelineLayout.get(),
        inputAssemblyStage,
        vertexStage,
        rasterizationStage,
        fragmentStage
    };

    m_onscreen.renderPipeline = m_device->createRenderPipeline(descriptor);
}

VulkanRenderPass& VulkanPipelineBarrierSample::getOffscreenRenderPass(Stage stage)
{
    VulkanRenderPassDescriptor vkdescriptor{};

    {
        VkAttachmentDescription attachmentDescription{};
        attachmentDescription.format = ToVkFormat(m_offscreen.renderTexture->getFormat());
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachmentDescription.samples = ToVkSampleCountFlagBits(m_sampleCount);

        vkdescriptor.attachmentDescriptions = { attachmentDescription };
    }

    {
        VkAttachmentReference attachmentReference{};
        attachmentReference.attachment = 0;
        attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VulkanSubpassDescription subpassDescription{};
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachments = { attachmentReference };

        vkdescriptor.subpassDescriptions = { subpassDescription };
    }

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = stage.srcStageMask;
    subpassDependency.dstStageMask = stage.dstStageMask;
    subpassDependency.srcAccessMask = stage.srcAccessMask;
    subpassDependency.dstAccessMask = stage.dstAccessMask;

    vkdescriptor.subpassDependencies = { subpassDependency };

    return downcast(m_device.get())->getRenderPass(vkdescriptor);
}

VulkanFramebuffer& VulkanPipelineBarrierSample::getOffscreenFramebuffer(TextureView& renderView)
{
    VulkanFramebufferDescriptor vkdescriptor{};
    vkdescriptor.renderPass = getOffscreenRenderPass(m_stage).getVkRenderPass();
    vkdescriptor.width = m_swapchain->getWidth();
    vkdescriptor.height = m_swapchain->getHeight();
    vkdescriptor.layers = 1;
    vkdescriptor.attachments.push_back(downcast(renderView).getVkImageView());

    return downcast(m_device.get())->getFrameBuffer(vkdescriptor);
}

} // namespace jipu

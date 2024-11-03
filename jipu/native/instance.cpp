#include "instance.h"

#include "vulkan/vulkan_instance.h"

#if defined(__ANDROID__) || defined(ANDROID)
#include "spdlog/sinks/android_sink.h"
#else
#include "spdlog/sinks/stdout_color_sinks.h"
#endif
#include <spdlog/spdlog.h>

namespace jipu
{

static std::shared_ptr<spdlog::logger> getLogger()
{
#if defined(__ANDROID__) || defined(ANDROID)
    std::string tag = "spdlog-android";
    static auto logger = spdlog::android_logger_mt("jipu");
#else
    static auto logger = spdlog::stdout_color_mt("jipu");
#endif

    return logger;
}

std::unique_ptr<Instance> Instance::create(const InstanceDescriptor& descriptor)
{
    switch (descriptor.type)
    {
    case InstanceType::kVulkan:
        return std::make_unique<VulkanInstance>(descriptor);
    default:
        spdlog::error("Unsupported instance type requested");
        return nullptr;
    }

    return nullptr;
}

Instance::Instance()
{
    spdlog::set_default_logger(getLogger());
    spdlog::set_level(spdlog::level::trace);
}

Instance::~Instance()
{
    spdlog::set_default_logger(nullptr);
}

} // namespace jipu

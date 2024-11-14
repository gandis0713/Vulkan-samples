#include "adapter.h"

#include "vulkan/vulkan_adapter.h"

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

std::unique_ptr<Adapter> Adapter::create(const AdapterDescriptor& descriptor)
{
    switch (descriptor.type)
    {
    case AdapterType::kVulkan:
        return std::make_unique<VulkanAdapter>(descriptor);
    default:
        spdlog::error("Unsupported instance type requested");
        return nullptr;
    }

    return nullptr;
}

Adapter::Adapter()
{
    spdlog::set_default_logger(getLogger());
    spdlog::set_level(spdlog::level::trace);
}

Adapter::~Adapter()
{
    spdlog::set_default_logger(nullptr);
}

} // namespace jipu

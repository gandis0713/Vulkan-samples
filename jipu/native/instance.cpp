#include "instance.h"

#include "vulkan_adapter.h"

#if defined(__ANDROID__) || defined(ANDROID)
#include "spdlog/sinks/android_sink.h"
#else
#include "spdlog/sinks/stdout_color_sinks.h"
#endif
#include <spdlog/spdlog.h>

namespace jipu
{
std::unique_ptr<Instance> Instance::create(const InstanceDescriptor& descriptor)
{
    auto instance = new Instance(descriptor);

    // TODO: change logger
    {
#if defined(__ANDROID__) || defined(ANDROID)
        std::string tag = "spdlog-android";
        static auto logger = spdlog::android_logger_mt("jipu");
#else
        static auto logger = spdlog::stdout_color_mt("jipu");
#endif
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::trace);
    }

    return std::unique_ptr<Instance>(instance);
}

Instance::Instance(const InstanceDescriptor& descriptor) noexcept
    : m_descriptor(descriptor)
{
}

Instance::~Instance()
{
    spdlog::set_default_logger(nullptr);
}

std::unique_ptr<Adapter> Instance::createAdapter(const AdapterDescriptor& descriptor)
{
    switch (descriptor.type)
    {
    case BackendAPI::kVulkan:
        return std::make_unique<VulkanAdapter>(this, descriptor);
    default:
        spdlog::error("Unsupported instance type requested");
        return nullptr;
    }

    return nullptr;
}

} // namespace jipu

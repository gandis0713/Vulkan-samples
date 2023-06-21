#include "vkt/gpu/driver.h"

#include "gpu/vulkan/vulkan_driver.h"

#if defined(__ANDROID__) || defined(ANDROID)
    #include "spdlog/sinks/android_sink.h"
#else
    #include "spdlog/sinks/stdout_color_sinks.h"
#endif
#include "utils/log.h"

namespace vkt
{

std::unique_ptr<Driver> Driver::create(const DriverDescriptor& descriptor)
{
    switch (descriptor.type)
    {
    case DriverType::VULKAN:
        return std::make_unique<VulkanDriver>(descriptor);
    default:
        break;
    }

    return nullptr;
}

Driver::Driver(const DriverDescriptor& descriptor)
{

#if defined(__ANDROID__) || defined(ANDROID)
    std::string tag = "spdlog-android";
    auto logger = spdlog::android_logger_mt("vkt", tag);
#else
    auto logger = spdlog::stdout_color_mt("vkt");
#endif
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::trace);
}

} // namespace vkt

#include "adreno_sampler.h"

#include "adreno_counter.h"
#include "adreno_expression.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace hpc
{
namespace adreno
{

AdrenoSampler::AdrenoSampler(AdrenoGPU& gpu, std::unique_ptr<hpc::backend::Sampler> sampler, const SamplerDescriptor& descriptor)
    : Sampler()
    , m_gpu(gpu)
    , m_sampler(std::move(sampler))
    , m_descriptor(descriptor)
{
}

std::error_code AdrenoSampler::start()
{
    spdlog::debug("start sampling {}", __func__);

    return m_sampler->start();
}

std::error_code AdrenoSampler::stop()
{
    spdlog::debug("stop sampling {}", __func__);

    return m_sampler->stop();
}

std::vector<Sample> AdrenoSampler::samples(std::unordered_set<Counter> counters)
{
    if (counters.empty())
        counters = m_descriptor.counters;

    std::unordered_set<hpc::backend::Counter> adrenoCounters{};
    for (const auto counter : counters)
    {
        auto backendCounter = convertCounter(counter);
        for (const auto backendCounter : backendCounter)
        {
            adrenoCounters.insert(backendCounter);
        }
    }

    auto adrenoSamples = m_sampler->sample(adrenoCounters);

    std::vector<hpc::Sample> samples{};
    for (const auto counter : counters)
    {
        switch (counter)
        {
        case Counter::NonFragmentUtilization:
            samples.push_back(expression::nonFragmentUtilization(counter, adrenoSamples));
            break;
        case Counter::FragmentUtilization:
            samples.push_back(expression::fragmentUtilization(counter, adrenoSamples));
            break;
        default:
            samples.push_back(expression::convert(counter, adrenoSamples));
            spdlog::error("Unsupported counter: {}", static_cast<uint32_t>(counter));
            break;
        }
    }

    return samples;
}

} // namespace adreno
} // namespace hpc
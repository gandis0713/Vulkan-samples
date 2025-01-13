#include "fps.h"
#include <numeric>

using namespace std::chrono;

namespace jipu
{

FPS::FPS()
    : m_time(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()))
{
}

float FPS::current()
{
    if (m_fps.empty())
        return 0;

    return m_fps[m_fps.size() - 1];
}

const std::deque<float>& FPS::getAll()
{
    return m_fps;
}

float FPS::min()
{
    return *std::min_element(m_fps.begin(), m_fps.end());
}

float FPS::max()
{
    return *std::max_element(m_fps.begin(), m_fps.end());
}

float FPS::average()
{
    return std::accumulate(m_fps.begin(), m_fps.end(), 0.0f) / m_fps.size();
}

void FPS::clear()
{
    m_time = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch());
    m_frame = 0;
    m_fps.clear();
}

void FPS::update()
{
    auto currentTime = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch());
    auto durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_time).count();

    ++m_frame;
    if (durationTime > 1000)
    {
        m_fps.push_back(m_frame * 1000.0 / durationTime);
        m_time = currentTime;
        m_frame = 0;
    }
}

} // namespace jipu

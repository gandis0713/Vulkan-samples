#include <chrono>
#include <deque>

namespace jipu
{

class FPS
{
public:
    FPS();

public:
    void update();
    float current();
    float min();
    float max();
    float average();
    const std::deque<float>& getAll();
    void clear();

private:
    std::chrono::milliseconds m_time = std::chrono::milliseconds::zero();
    uint64_t m_frame = 0;
    std::deque<float> m_fps{};
};

} // namespace jipu

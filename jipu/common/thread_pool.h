#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>

namespace jipu
{

class ThreadPool final
{
public:
    ThreadPool() = delete;
    ThreadPool(size_t numberOfThreads);
    ~ThreadPool();

public:
    std::future<void> enqueue(std::function<void()> func);
    void stop();

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_taskMutex;
    std::condition_variable m_condition;
    bool m_stop = false;
};

} // namespace jipu
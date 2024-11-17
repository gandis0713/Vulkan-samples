#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
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

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> tasks;
    std::mutex taskMutex;
    std::condition_variable condition;
    bool stop = false;
};

} // namespace jipu
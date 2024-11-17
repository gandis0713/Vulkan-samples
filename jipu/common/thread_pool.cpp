#include "thread_pool.h"

namespace jipu
{

ThreadPool::ThreadPool(size_t numberOfThreads)
{
    for (size_t i = 0; i < numberOfThreads; ++i)
    {
        m_threads.emplace_back([this]() {
            while (true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(m_taskMutex);
                    m_condition.wait(lock, [this]() {
                        return m_stop || !m_tasks.empty();
                    });

                    if (m_stop && m_tasks.empty())
                        return;

                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }

                // 작업 실행
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_stop = true;
    }

    m_condition.notify_all();
    for (std::thread& worker : m_threads)
    {
        worker.join();
    }
}

std::future<void> ThreadPool::enqueue(std::function<void()> func)
{
    auto task = std::make_shared<std::packaged_task<void()>>(std::bind(func));
    std::future<void> future = task->get_future();
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);
        m_tasks.emplace([task]() { (*task)(); });
    }

    m_condition.notify_one();

    return future;
}

} // namespace jipu
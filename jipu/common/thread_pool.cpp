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
                    std::unique_lock<std::mutex> lock(taskMutex);
                    condition.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty())
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();
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
        std::unique_lock<std::mutex> lock(taskMutex);
        stop = true;
    }

    condition.notify_all();
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
        std::unique_lock<std::mutex> lock(taskMutex);
        tasks.emplace([task]() { (*task)(); });
    }

    condition.notify_one();

    return future;
}

} // namespace jipu
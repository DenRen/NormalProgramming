#include <thread>
#include <deque>
#include <mutex>
#include <vector>
#include <algorithm>
#include <optional>
#include <condition_variable>

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

template <typename T>
class ThreadSafeDeque
{
    std::deque<T> m_deque;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;

public:
    ThreadSafeDeque() = default;

    void PushBack(T value)
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_deque.push_back(std::move(value));

        if (GetSize() == 1)
            m_cond_var.notify_one();
    }

    std::optional<T> PopFrontNonBlock()
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (GetSize() == 0)
            return std::nullopt;

        T val = std::move(m_deque.front());
        m_deque.pop_front();
        return val;
    }

    T PopFrontBlock()
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (GetSize() == 0)
        {
            do {
                m_mutex.unlock();
                {
                    std::unique_lock<std::mutex> lock_wait{m_mutex};
                    m_cond_var.wait(lock_wait, [&] { return GetSize() > 0; });
                }
                m_mutex.lock();
            } while (GetSize() == 0);
        }

        T val = std::move(m_deque.front());
        m_deque.pop_front();
        return val;
    }

    std::size_t GetSize() const noexcept
    {
        return m_deque.size();
    }
};

void task_exec(uint8_t value)
{
    printf("run\n");
    std::chrono::milliseconds dt(value);
    std::this_thread::sleep_for(dt);

    // I used iostream, because std::this_thread::get_id() used only operator <<
    std::cout << "completed " << std::this_thread::get_id() << ": " << (char)value << std::endl;
}

void thread_work(std::vector<ThreadSafeDeque<uint8_t>> &pools, const unsigned self_id)
{
    const unsigned num_threads = pools.size();

    while(true)
    {
        while(true)
        {
            std::optional<uint8_t> task = pools[self_id].PopFrontNonBlock();
            if (task.has_value())
                task_exec(*task);
            else
                break;
        }

        while (true)
        {
            unsigned max_num_tasks = 0;
            unsigned max_num_tasks_thread_id = 0;
            for (unsigned i_th = 0; i_th < num_threads; ++i_th)
            {
                if (i_th == self_id)
                    continue;

                if (const auto num_tasks = pools[i_th].GetSize(); num_tasks > max_num_tasks)
                {
                    max_num_tasks = num_tasks;
                    max_num_tasks_thread_id = i_th;
                }
            }

            if (max_num_tasks == 0)
            {
                uint8_t task = pools[self_id].PopFrontBlock();
                task_exec(task);

                break;
            }

            std::optional<uint8_t> task = pools[self_id].PopFrontNonBlock();
            if (task.has_value())
                task_exec(*task);

            if (pools[self_id].GetSize())
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Please, enter the number threads\n");
        return 0;
    }

    const unsigned num_threads = atoi(argv[1]);
    if (num_threads == 0)
    {
        printf("Incorrect number of threads\n");
        return 0;
    }

    std::vector<ThreadSafeDeque<uint8_t>> task_pools(num_threads);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (unsigned thread_id = 0; thread_id < num_threads; ++thread_id)
        threads.emplace_back(thread_work, std::ref(task_pools), thread_id);

    pollfd pfd{};
    pfd.events = POLLIN | POLLHUP;
    pfd.fd = STDIN_FILENO;

    const auto read_buf_size = 256;
    std::vector<uint8_t> task_buf(read_buf_size);

    // thread_id, number_task
    std::vector<std::pair<unsigned, unsigned>> number_current_tasks(num_threads);

    while (true)
    {
        if (poll(&pfd, 1, -1) == -1)
        {
            perror("poll");
            return errno;
        }

        auto readed_size = read(pfd.fd, task_buf.data(), task_buf.size());
        if (readed_size == -1)
        {
            perror("read");
            return errno;
        }

        for (unsigned th = 0; th < num_threads; ++th)
            number_current_tasks[th] = {th, task_pools[th].GetSize()};

        std::sort(number_current_tasks.begin(), number_current_tasks.end(),
                  [](const auto &lhs, const auto &rhs) noexcept
                  {
                      return lhs.second < rhs.second;
                  });

        unsigned pos = 0;
        for (unsigned i_task = 0; i_task + 1 < readed_size; ++i_task) // Remove '\n'
        {
            task_pools[number_current_tasks[pos].first].PushBack(task_buf[i_task]);
            printf("setted task %c to %u\n", task_buf[i_task], pos);
            
            ++number_current_tasks[pos].second;
            if (pos + 1 == num_threads ||
                number_current_tasks[pos + 1].second >= number_current_tasks[pos].second)
            {
                pos = 0;
            }
            else
            {
                ++pos;
            }
        }
    }
}
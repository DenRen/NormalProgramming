#pragma once

#include <thread>
#include <atomic>
#include <array>
#include <vector>

struct HazardPointers
{
    std::atomic<std::thread::id> m_thread_id;
    std::vector<std::atomic<void*>> m_hazard_pointers;
    std::vector<std::atomic<void*>> m_retired_pointers;

    void Init(unsigned num_hazard_pointers, unsigned num_retired_pointers)
    {
        auto init = [](auto& m_ptrs, unsigned num_ptrs)
        {
            std::vector<std::atomic<void*>> ptrs(num_ptrs);
            for (auto& ptr : ptrs)
                ptr.store(nullptr);

            m_ptrs = std::move(ptrs);
        };
        
        init(m_hazard_pointers, num_hazard_pointers);
        init(m_retired_pointers, num_retired_pointers);
    }
};

class HazardPointerStorage
{
    const unsigned m_max_num_threads;
    const unsigned m_max_num_guarded_ptrs;

    std::vector<HazardPointers> m_hptr_storage;

public:
    HazardPointerStorage(unsigned max_num_threads, unsigned max_num_guarded_ptrs)
        : m_max_num_threads{ max_num_threads }
        , m_max_num_guarded_ptrs{ max_num_guarded_ptrs }
        , m_hptr_storage( max_num_threads )
    {
        const unsigned num_hazard_pointers = max_num_guarded_ptrs;
        const unsigned num_retired_pointers = 2 * max_num_threads * num_hazard_pointers;

        for (auto& hptr : m_hptr_storage)
            hptr.Init(num_hazard_pointers, num_retired_pointers);
    }

    unsigned GetMaxNumberThreads() const noexcept { return m_max_num_threads; }
    unsigned GetMaxNumberGuardedPtrs() const noexcept { return m_max_num_guarded_ptrs; }
    const std::vector<HazardPointers>& GetAllHazardPointers() const noexcept { return m_hptr_storage; }

    HazardPointers& AddNewThread(std::thread::id id)
    {
        std::thread::id empty_id;
        if (id == empty_id)
            throw std::invalid_argument("Tryed to add empty id");

        for (auto& hptr : m_hptr_storage)
        {
            if (hptr.m_thread_id.load() == empty_id)
            {
                if (hptr.m_thread_id.compare_exchange_strong(empty_id, id))
                    return hptr;
                
                empty_id = std::thread::id{};
            }
        }

        throw std::runtime_error("Failed to add new thread in HPStorage!");
    }
};

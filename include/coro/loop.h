#pragma once

#include "handle.h"
#include "task.h"
#include <queue>
#include <chrono>
#include <vector>
#include <algorithm>
#include <thread>

namespace coro
{
    class Loop
    {
        using MS = std::chrono::milliseconds;
        using delayed_handle = std::pair<MS, handle_wrapper>;
        using clock = std::chrono::steady_clock;

    public:
        Loop()
        {
            startup_time = std::chrono::duration_cast<MS>(clock::now().time_since_epoch());
        }
        ~Loop() = default;
        Loop(Loop const&) = delete;
        Loop(Loop&&) = delete;
        Loop& operator=(Loop const&) = delete;
        Loop& operator=(Loop&&) = delete;

        void call(handle& _handle)
        {
            handles.push({ _handle.get_handle_id(), &_handle });
        }

        template<typename Ret>
        void call(task<Ret>& _task)
        {
            call(_task.promise());
        }

        template<typename Rep, typename Period, typename Ret>
        void call_after(std::chrono::duration<Rep, Period> delay, task<Ret>& _task)
        {
            auto t = std::chrono::duration_cast<MS>(delay) + now();
            delayed_handles.emplace_back(t, handle_wrapper{ _task.promise().get_handle_id(), &_task.promise() });
            std::ranges::push_heap(delayed_handles, std::ranges::greater{}, &delayed_handle::first);  // min heap
        }

        void run_until_complete()
        {
            while(!is_stop()) run_once();
        }

    private:
        bool is_stop()
        {
            return handles.empty() && delayed_handles.empty();
        }

        void run_once()
        {
            auto current = now();
            while (!delayed_handles.empty())
            {
                auto&& [t, h] = delayed_handles[0];
                if (t >= current) break;
                handles.push(h);
                std::ranges::pop_heap(delayed_handles, std::ranges::greater{}, &delayed_handle::first);
                delayed_handles.pop_back();
            }

            for (size_t i = 0, n = handles.size(); i < n; i++)
            {
                auto [id, h] = handles.front();
                handles.pop();
                h->run();
            }
        }

        MS now()
        {
            return std::chrono::duration_cast<MS>(clock::now().time_since_epoch()) - startup_time;
        }

    private:
        std::queue<handle_wrapper> handles;

        MS startup_time;
        std::vector<delayed_handle> delayed_handles;  // minimum time heap
    };
}

#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <memory>

#include "Asserts.hpp"
#include "BackgroundWorker.hpp"

namespace Motion
{
    class MainThreadDispatcher : public IExecutor 
    {
        public:
            static MainThreadDispatcher& Instance() 
            {
                static MainThreadDispatcher s;
                return s;
            }

            static std::shared_ptr<IExecutor> Shared() 
            {
                // Non-owning shared_ptr to the singleton Instance()
                static std::shared_ptr<IExecutor> s(&Instance(), [](IExecutor*){});
                return s;
            }


            void Post(std::function<void()> fn) override 
            { 
                Enqueue(std::move(fn)); 
            }

            void Enqueue(std::function<void()> fn) 
            {
                std::scoped_lock lk(m_mtx);
                m_q.push(std::move(fn));
            }

            void Dispatch() 
            {
                std::queue<std::function<void()>> local;
                {
                    std::scoped_lock lk(m_mtx);
                    std::swap(local, m_q);
                }
                while (!local.empty()) 
                {
                    auto fn = std::move(local.front());
                    local.pop();
                    try { fn(); } catch (...) {}
                }
            }

        private:
            MainThreadDispatcher() = default;

            std::mutex m_mtx;
            std::queue<std::function<void()>> m_q;
    };
}



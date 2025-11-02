#pragma once

#include <thread>
#include <atomic>
#include <future>
#include <mutex>
#include <deque>
#include <memory>
#include <utility>
#include <type_traits>
#include <functional>
#include <exception>
#include <glad/glad.h>

#include "Window.hpp"
#include "Renderer.hpp"

namespace Motion
{
    template<typename T>
    class ThreadSafeQueue
    {
        public:
            void Push(T v)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _queue.push_back(std::move(v));
            }

            bool TryPopping(T& out)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_queue.empty()) return false;
                out = std::move(_queue.front());
                _queue.pop_front();
                return true;
            }

            bool Empty() const
            {
                std::lock_guard<std::mutex> lock(_mutex);
                return _queue.empty();
            }

        private:
            mutable std::mutex _mutex;
            std::deque<T> _queue;
        };

        struct IJob
        {
            virtual ~IJob() = default;
            virtual void Run() = 0;
        };

        struct ICompletion
        {
            virtual ~ICompletion() = default;
            virtual bool Adapt() = 0;
        };

        template <class Fn, class T>
        struct Job final : IJob 
        {
            Fn Function;
            std::shared_ptr<std::promise<T>> Promise;
            std::shared_ptr<ThreadSafeQueue<std::unique_ptr<ICompletion>>> Completions;
            bool UseGLFence = false;

            template <class F>
            Job(F&& f,
                std::shared_ptr<std::promise<T>> p,
                std::shared_ptr<ThreadSafeQueue<std::unique_ptr<ICompletion>>> c,
                bool useFence)
                : Function(std::forward<F>(f))
                , Promise(std::move(p))
                , Completions(std::move(c))
                , UseGLFence(useFence) {}

            void Run() override 
            {
                try 
                {
                    if constexpr (std::is_void_v<T>) 
                    {
                        Function();

                        if (UseGLFence) 
                        {
                            GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                            glFlush();

                            struct Completion final : ICompletion 
                            {
                                std::shared_ptr<std::promise<T>> P;
                                GLsync Fence{};
                                
                                Completion(std::shared_ptr<std::promise<T>> p, GLsync f)
                                    : P(std::move(p)), Fence(f) {}
                                
                                ~Completion() override 
                                { 
                                    if (Fence) 
                                    { 
                                        glDeleteSync(Fence); 
                                        Fence = nullptr; 
                                    } 
                                }

                                bool Adapt() override 
                                {
                                    if (!Fence) 
                                    { 
                                        P->set_value(); 
                                        return true; 
                                    }
                                    
                                    GLenum status = glClientWaitSync(Fence, 0, 0);
                                    if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) 
                                    {
                                        glDeleteSync(Fence); 
                                        Fence = nullptr;
                                        P->set_value(); 
                                        return true;
                                    }
                                    return false;
                                }
                            };

                            Completions->Push(std::make_unique<Completion>(Promise, fence));
                        } 
                        else 
                        {
                            struct CompletionImmediate final : ICompletion 
                            {
                                std::shared_ptr<std::promise<T>> P;
                                
                                explicit CompletionImmediate(std::shared_ptr<std::promise<T>> p) 
                                    : P(std::move(p)) {}
                                
                                bool Adapt() override 
                                { 
                                    P->set_value(); 
                                    return true; 
                                }
                            };

                            Completions->Push(std::make_unique<CompletionImmediate>(Promise));
                        }
                    } 
                    else
                    {
                        T result = Function();

                        if (UseGLFence) 
                        {
                            GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                            glFlush();

                            struct Completion final : ICompletion 
                            {
                                std::shared_ptr<std::promise<T>> P;
                                T R;
                                GLsync Fence{};
                                
                                Completion(std::shared_ptr<std::promise<T>> p, T&& r, GLsync f)
                                    : P(std::move(p)), R(std::move(r)), Fence(f) {}
                                
                                ~Completion() override 
                                { 
                                    if (Fence) 
                                    { 
                                        glDeleteSync(Fence); 
                                        Fence = nullptr; 
                                    } 
                                }

                                bool Adapt() override 
                                {
                                    if (!Fence) 
                                    { 
                                        P->set_value(std::move(R)); 
                                        return true; 
                                    }
                                    
                                    GLenum status = glClientWaitSync(Fence, 0, 0);
                                    if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) 
                                    {
                                        glDeleteSync(Fence); 
                                        Fence = nullptr;
                                        P->set_value(std::move(R)); 
                                        return true;
                                    }
                                    return false;
                                }
                            };

                            Completions->Push(std::make_unique<Completion>(Promise, std::move(result), fence));
                        }
                        else 
                        {
                            struct CompletionImmediate final : ICompletion 
                            {
                                std::shared_ptr<std::promise<T>> P;
                                T R;
                                
                                explicit CompletionImmediate(std::shared_ptr<std::promise<T>> p, T&& r)
                                    : P(std::move(p)), R(std::move(r)) {}
                                
                                bool Adapt() override 
                                { 
                                    P->set_value(std::move(R)); 
                                    return true; 
                                }
                            };

                            Completions->Push(std::make_unique<CompletionImmediate>(Promise, std::move(result)));
                        }
                    }
                } 
                catch (...) 
                {
                    struct Fail final : ICompletion 
                    {
                        std::shared_ptr<std::promise<T>> P;
                        
                        explicit Fail(std::shared_ptr<std::promise<T>> p) 
                            : P(std::move(p)) {}
                        
                        bool Adapt() override 
                        { 
                            try 
                            { 
                                P->set_exception(std::current_exception()); 
                            } 
                            catch (...) 
                            {
                                // Promise may already be satisfied or in invalid state
                                // Nothing we can do here - already in exception handler
                            }
                            return true; 
                        }
                    };

                    Completions->Push(std::make_unique<Fail>(Promise));
                }
            }
        };

        class LOADER
        {
        public:
            static void Create(NativeWindow sharedWindow)
            {
                auto& instance = Instance();
                std::lock_guard<std::mutex> lock(instance._initMutex);
                
                if (instance._loaderWindow) 
                    return;

                auto& windowManager = WindowManager::GetInstance();
                instance._loaderWindow = windowManager.Create("LoaderThread", false, sharedWindow);
                instance._context = IContext::GetContext();
            }

            static void Start()
            {
                auto& instance = Instance();
                std::lock_guard<std::mutex> lock(instance._initMutex);
                
                if (instance._running || !instance._loaderWindow || !instance._context)
                    return;

                instance._running = true;
                instance._thread = std::thread([&instance]()
                {
                    instance._context->MakeCurrent(instance._loaderWindow->GetNativeWindow());

                    while (instance._running)
                    {
                        std::unique_ptr<IJob> job;
                        if (instance._jobs.TryPopping(job))
                        {
                            job->Run();
                        }
                        else
                        {
                            std::this_thread::yield();
                        }
                    }

                    instance._context->ClearCurrent();
                });
            }

            static void Stop()
            {
                auto& instance = Instance();
                
                {
                    std::lock_guard<std::mutex> lock(instance._initMutex);
                    if (!instance._running) 
                        return;
                    
                    instance._running = false;
                }

                if (instance._thread.joinable())
                    instance._thread.join();

                DrainCompletions(instance);

                std::lock_guard<std::mutex> lock(instance._initMutex);
                if (instance._loaderWindow)
                {
                    auto& windowManager = WindowManager::GetInstance();
                    windowManager.Destroy(instance._loaderWindow->GetHandle());
                    instance._loaderWindow = nullptr;
                }
                instance._context = nullptr;
            }

            template <class Fn>
            static auto Submit(Fn&& func) -> std::future<std::invoke_result_t<Fn&>> 
            {
                using T = std::invoke_result_t<Fn&>;
                using F = std::decay_t<Fn>;
                
                auto& instance = Instance();
                
                static_assert(!std::is_reference_v<T>, 
                    "Submit functor must not return a reference type.");
                
                auto promise = std::make_shared<std::promise<T>>();
                auto future = promise->get_future();

                const bool useFence = (instance._context != nullptr) && 
                                    (Renderer::GetAPI() == RenderingAPI::OpenGL);

                auto job = std::make_unique<Job<F, T>>(
                    std::forward<Fn>(func), 
                    promise, 
                    instance._completions, 
                    useFence
                );
                
                instance._jobs.Push(std::move(job));
                return future;
            }

            static void FeedBack()
            {
                auto& instance = Instance();
                
                // Process pending completions that are waiting on GPU fences
                if (!instance._pending.empty())
                {
                    auto it = instance._pending.begin();
                    while (it != instance._pending.end())
                    {
                        bool done = false;
                        try 
                        {
                            done = (*it)->Adapt();
                        } 
                        catch (...) 
                        {
                            // If adaptation throws, consider it done to avoid getting stuck
                            done = true;
                        }
                        
                        if (done)
                            it = instance._pending.erase(it);
                        else
                            ++it;
                    }
                }

                // Process new completions from the queue
                std::unique_ptr<ICompletion> completion;
                size_t processed = 0;
                constexpr size_t kMaxPerFrame = 256;

                while (processed < kMaxPerFrame && instance._completions->TryPopping(completion))
                {
                    bool done = false;
                    try 
                    {
                        done = completion->Adapt();
                    } 
                    catch (...) 
                    {
                        // If adaptation throws, consider it done
                        done = true;
                    }
                    
                    if (!done)
                    {
                        instance._pending.push_back(std::move(completion));
                    }
                    ++processed;
                }
            }

        private:
            LOADER()
                : _completions(std::make_shared<ThreadSafeQueue<std::unique_ptr<ICompletion>>>())
            {}

            static LOADER& Instance()
            {
                static LOADER instance;
                return instance;
            }

            static void DrainCompletions(LOADER& instance)
            {
                // Try multiple passes to complete all pending operations
                constexpr int kMaxDrainPasses = 4;
                
                for (int pass = 0; pass < kMaxDrainPasses; ++pass)
                {
                    // Process existing pending completions
                    auto it = instance._pending.begin();
                    while (it != instance._pending.end())
                    {
                        bool done = false;
                        try
                        {
                            done = (*it)->Adapt();
                        }
                        catch (...)
                        {
                            done = true;
                        }
                        
                        if (done)
                            it = instance._pending.erase(it);
                        else
                            ++it;
                    }
                    
                    // Process new completions from the queue
                    std::unique_ptr<ICompletion> completion;
                    while (instance._completions->TryPopping(completion))
                    {
                        bool done = false;
                        try
                        {
                            done = completion->Adapt();
                        }
                        catch (...)
                        {
                            done = true;
                        }
                        
                        if (!done)
                        {
                            instance._pending.push_back(std::move(completion));
                        }
                    }
                    
                    // If everything is complete, we're done
                    if (instance._pending.empty())
                        break;
                }

                // After all drain attempts, abandon any remaining pending completions
                // These are likely GPU operations that won't complete without an active context
                if (!instance._pending.empty())
                {
                    // Set broken_promise exception on any remaining futures
                    for (auto& completion : instance._pending)
                    {
                        try
                        {
                            // Force completion - this may fail if promise is already satisfied
                            completion->Adapt();
                        }
                        catch (...)
                        {
                            // Ignore errors during forced cleanup
                        }
                    }
                    instance._pending.clear();
                }
            }

        private:
            std::atomic_bool _running{false};
            std::mutex _initMutex;  // Protects initialization/shutdown
            std::thread _thread;
            
            ThreadSafeQueue<std::unique_ptr<IJob>> _jobs;
            std::shared_ptr<ThreadSafeQueue<std::unique_ptr<ICompletion>>> _completions;
            std::deque<std::unique_ptr<ICompletion>> _pending;

            std::shared_ptr<IWindow> _loaderWindow{nullptr};
            std::shared_ptr<IContext> _context{nullptr};
    };
}
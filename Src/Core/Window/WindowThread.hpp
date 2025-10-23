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
    class MPSC
    {
    public:
        void Push(T v)
        {
            std::lock_guard<std::mutex> l(_m);
            _q.push_back(std::move(v));
        }

        bool TryPoping(T& out)
        {
            std::lock_guard<std::mutex> l(_m);
            if (_q.empty()) return false;
            out = std::move(_q.front());
            _q.pop_front();
            return true;
        }

        bool Empty() const
        {
            std::lock_guard<std::mutex> l(_m);
            return _q.empty();
        }

    private:
        mutable std::mutex _m;
        std::deque<T> _q;
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
        std::shared_ptr<MPSC<std::unique_ptr<ICompletion>>> Completions;
        bool UseGLFence = false;

        template <class F>
        Job(F&& f,
            std::shared_ptr<std::promise<T>> p,
            std::shared_ptr<MPSC<std::unique_ptr<ICompletion>>> c,
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
                            ~Completion() override { if (Fence) { glDeleteSync(Fence); Fence = nullptr; } }

                            bool Adapt() override 
                            {
                                if (!Fence) { P->set_value(); return true; }
                                GLenum s = glClientWaitSync(Fence, 0, 0);
                                if (s == GL_ALREADY_SIGNALED || s == GL_CONDITION_SATISFIED) {
                                    glDeleteSync(Fence); Fence = nullptr;
                                    P->set_value(); return true;
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
                            explicit CompletionImmediate(std::shared_ptr<std::promise<T>> p) : P(std::move(p)) {}
                            bool Adapt() override { P->set_value(); return true; }
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

                        struct Completion final : ICompletion {
                            std::shared_ptr<std::promise<T>> P;
                            T R;
                            GLsync Fence{};
                            Completion(std::shared_ptr<std::promise<T>> p, T&& r, GLsync f)
                                : P(std::move(p)), R(std::move(r)), Fence(f) {}
                            ~Completion() override { if (Fence) { glDeleteSync(Fence); Fence = nullptr; } }

                            bool Adapt() override 
                            {
                                if (!Fence) { P->set_value(std::move(R)); return true; }
                                GLenum s = glClientWaitSync(Fence, 0, 0);
                                if (s == GL_ALREADY_SIGNALED || s == GL_CONDITION_SATISFIED) {
                                    glDeleteSync(Fence); Fence = nullptr;
                                    P->set_value(std::move(R)); return true;
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
                            bool Adapt() override { P->set_value(std::move(R)); return true; }
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
                    explicit Fail(std::shared_ptr<std::promise<T>> p) : P(std::move(p)) {}
                    bool Adapt() override { try { P->set_exception(std::current_exception()); } catch (...) {} return true; }
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
                auto& wm = WindowManager::GetInstance();
                auto& L = Instance();

                if (L._LoaderWindow) return;
                L._LoaderWindow = wm.Create("LoaderThread", false, sharedWindow);
                L._Context = IContext::GetContext();
            }

            static void Start()
            {
                auto& L = Instance();
                if (L._Running || !L._LoaderWindow || !L._Context)
                    return;

                L._Running = true;
                L._Thread = std::thread([&L]
                {
                    L._Context->MakeCurrent(L._LoaderWindow->GetNativeWindow());

                    while (L._Running)
                    {
                        std::unique_ptr<IJob> job;
                        if (L._Jobs.TryPoping(job))
                        {
                            job->Run();
                        }
                        else
                        {
                            std::this_thread::yield();
                        }
                    }

                    L._Context->ClearCurrent();
                });
            }

            static void Stop()
            {
                auto& L = Instance();
                if (!L._Running) return;

                L._Running = false;
                if (L._Thread.joinable())
                    L._Thread.join();

                DrainCompletions(L);
                L._Context->ClearCurrent();

                auto& windowManager = WindowManager::GetInstance();
                windowManager.Destroy(L._LoaderWindow->GetHandle());
                L._LoaderWindow = nullptr;
            }

            template <class Fn>
            static auto Submit(Fn&& func) -> std::future<std::invoke_result_t<Fn&>> 
            {
                using T  = std::invoke_result_t<Fn&>;
                using F  = std::decay_t<Fn>;                 
                auto& L  = Instance();
                
                static_assert(!std::is_reference_v<T>, "Submit functor must not return a reference type.");
                auto promise = std::make_shared<std::promise<T>>();
                auto future  = promise->get_future();

                const bool useFence = (L._Context != nullptr) && (Renderer::GetAPI() == RenderingAPI::OpenGL);

                auto job = std::make_unique<Job<F, T>>(std::forward<Fn>(func), promise, L._Completions, useFence);
                L._Jobs.Push(std::move(job));
                return future;
            }

            static void FeedBack()
            {
                auto& L = Instance();
                if (!L._Pending.empty())
                {
                    auto it = L._Pending.begin();
                    while (it != L._Pending.end())
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
                        if (done) it = L._Pending.erase(it); else ++it;
                    }
                }

                std::unique_ptr<ICompletion> completion;
                size_t processed = 0;
                constexpr size_t kMaxPerFrame = 256;

                while (processed < kMaxPerFrame && L._Completions->TryPoping(completion))
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
                        L._Pending.push_back(std::move(completion));
                    }
                    ++processed;
                }
            }

        private:
            LOADER()
                : _Completions(std::make_shared<MPSC<std::unique_ptr<ICompletion>>>()){}

            static LOADER& Instance()
            {
                static LOADER instance;
                return instance;
            }

            static void DrainCompletions(LOADER& L)
            {
                for (int pass = 0; pass < 4; ++pass)
                {
                    auto it = L._Pending.begin();
                    while (it != L._Pending.end())
                    {
                        if ((*it)->Adapt()) it = L._Pending.erase(it); else ++it;
                    }
                    std::unique_ptr<ICompletion> completion;
                    while (L._Completions->TryPoping(completion))
                    {
                        if (!completion->Adapt())
                        {
                            L._Pending.push_back(std::move(completion));
                        }
                    }
                    if (L._Pending.empty()) break;
                }

                for (auto& c : L._Pending)
                {
                    struct Drop final : ICompletion
                    {
                        bool Adapt() override { return true; }
                    };
                }
                
                L._Pending.clear();
            }

        private:
            std::atomic_bool _Running{false};
            std::thread _Thread;
            MPSC<std::unique_ptr<IJob>> _Jobs;
            std::shared_ptr<MPSC<std::unique_ptr<ICompletion>>> _Completions;
            std::deque<std::unique_ptr<ICompletion>> _Pending;

            std::shared_ptr<IWindow> _LoaderWindow{ nullptr };
            std::shared_ptr<IContext> _Context{ nullptr };
    };
}

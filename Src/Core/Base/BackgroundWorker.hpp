#pragma once
#include <atomic>
#include <any>
#include <functional>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <stdexcept>

namespace Motion
{
    class IExecutor 
    {
        public:
            virtual ~IExecutor() = default;
            virtual void Post(std::function<void()> fn) = 0;
    };

    class InlineExecutor final : public IExecutor 
    {
        public:
            void Post(std::function<void()> fn) override { fn(); }
    };

    class BackgroundWorker 
    {
        public:
            // === Properties ===
            bool WorkerReportsProgress{false};
            bool WorkerSupportsCancellation{false};

            bool IsBusy() const noexcept { return m_isBusy.load(std::memory_order_acquire); }
            bool CancellationPending() const noexcept { return m_cancelPending.load(std::memory_order_acquire); }

            // === Events ===
            using DoWorkHandler = std::function<std::any(BackgroundWorker&, const std::any&, std::atomic_bool&)>;
            using ProgressHandler = std::function<void(BackgroundWorker&, int, const std::any&)>;
            using CompletedHandler = std::function<void(BackgroundWorker&, const std::any&, bool, std::exception_ptr)>;

            void SetDoWork(DoWorkHandler h)             { std::scoped_lock lk(m_handlerMutex); m_doWork = std::move(h); }
            void SetProgressChanged(ProgressHandler h)  { std::scoped_lock lk(m_handlerMutex); m_progress = std::move(h); }
            void SetRunWorkerCompleted(CompletedHandler h){ std::scoped_lock lk(m_handlerMutex); m_completed = std::move(h); }

            void SetCallbackExecutor(std::shared_ptr<IExecutor> exec) 
            {
                if (!exec) exec = std::make_shared<InlineExecutor>();
                std::scoped_lock lk(m_handlerMutex);
                m_callbackExecutor = std::move(exec);
            }

            BackgroundWorker() : m_callbackExecutor(std::make_shared<InlineExecutor>()) {}
            ~BackgroundWorker() 
            {
                if (IsBusy()) 
                {
                    CancelAsync();
                    JoinWorker();
                }
            }

            // === Methods ===
            bool RunWorkerAsync(std::any argument = {}) 
            {
                bool expected = false;
                if (!m_isBusy.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) 
                {
                    return false;
                }

                if (!WorkerSupportsCancellation) m_cancelPending.store(false, std::memory_order_release);
                m_result.reset();
                m_error = nullptr;

                DoWorkHandler doWorkCopy;
                std::shared_ptr<IExecutor> execCopy;
                {
                    std::scoped_lock lk(m_handlerMutex);
                    doWorkCopy = m_doWork;
                    execCopy = m_callbackExecutor;
                }

                if (!doWorkCopy && !HasOverriddenDoWork()) 
                {
                    m_isBusy.store(false, std::memory_order_release);
                    throw std::logic_error("BackgroundWorker: DoWork handler not set and OnDoWork not overridden.");
                }

                m_worker = std::jthread([this, argument = std::move(argument), doWorkCopy, execCopy]() mutable 
                {
                    try 
                    {
                        std::any r = OnDoWork(argument, doWorkCopy);
                        m_result = std::make_unique<std::any>(std::move(r));
                        PostCompleted(execCopy, CancellationPending(), nullptr);

                    } catch (...) 
                    {
                        m_error = std::current_exception();
                        PostCompleted(execCopy, false, m_error);
                    }

                    m_isBusy.store(false, std::memory_order_release);
                    m_cancelPending.store(false, std::memory_order_release);
                });

                return true;
            }

            void CancelAsync() 
            {
                if (!WorkerSupportsCancellation) return;
                m_cancelPending.store(true, std::memory_order_release);
            }

            void ReportProgress(int percent, std::any userState = {}) 
            {
                if (!WorkerReportsProgress || !IsBusy()) return;

                ProgressHandler progressCopy;
                std::shared_ptr<IExecutor> execCopy;
                {
                    std::scoped_lock lk(m_handlerMutex);
                    progressCopy = m_progress;
                    execCopy = m_callbackExecutor;
                }
                if (!progressCopy) return;

                execCopy->Post([this, percent, us = std::move(userState), progressCopy]() mutable 
                {
                    try 
                    {
                        OnProgressChanged(percent, us, progressCopy);

                    } catch (...) {}
                });
            }

            void Join() { JoinWorker(); }

        protected:
            virtual std::any OnDoWork(const std::any& argument, const DoWorkHandler& fallback) 
            {
                if (fallback) return fallback(*this, argument, m_cancelPending);
                throw std::logic_error("OnDoWork not implemented.");
            }

            virtual void OnProgressChanged(int percent, const std::any& userState, const ProgressHandler& fallback) 
            {
                if (fallback) fallback(*this, percent, userState);
            }

            virtual void OnRunWorkerCompleted(const std::any& result, bool cancelled, std::exception_ptr error, const CompletedHandler& fallback) 
            {
                if (fallback) fallback(*this, result, cancelled, error);
            }

        private:
            bool HasOverriddenDoWork() const noexcept { return false; }

            void PostCompleted(const std::shared_ptr<IExecutor>& exec, bool cancelled, std::exception_ptr err) 
            {
                CompletedHandler completedCopy;
                std::shared_ptr<std::any> resultCopy;
                {
                    std::scoped_lock lk(m_handlerMutex);
                    completedCopy = m_completed;
                    if (m_result) 
                    {
                        resultCopy = std::make_shared<std::any>(*m_result);
                    }
                }

                exec->Post([this, cancelled, err, completedCopy, resultCopy]() {
                    try {
                        const std::any empty{};
                        OnRunWorkerCompleted(resultCopy ? *resultCopy : empty, cancelled, err, completedCopy);
                    } catch (...) {}
                });

            }

            void JoinWorker()
            {
                if (m_worker.joinable()) m_worker.join();
            }

        private:
            mutable std::mutex m_handlerMutex;
            DoWorkHandler m_doWork;
            ProgressHandler m_progress;
            CompletedHandler m_completed;
            std::shared_ptr<IExecutor> m_callbackExecutor;

            std::jthread m_worker;
            std::atomic_bool m_isBusy{false};
            std::atomic_bool m_cancelPending{false};

            std::unique_ptr<std::any> m_result;
            std::exception_ptr m_error{};
    };

}


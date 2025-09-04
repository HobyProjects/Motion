#pragma once
#include <any>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Asserts.hpp"
#include "BackgroundWorker.hpp"

namespace Motion
{
    class TaskManager 
    {
        public:
            using TaskId = std::uint64_t;

            enum class TaskStatus { Pending, Running, Completed, Cancelled, Faulted };

            struct TaskSpec 
            {
                BackgroundWorker::DoWorkHandler DoWork;  // required
                std::any Argument{};
                bool WorkerReportsProgress{true};
                bool WorkerSupportsCancellation{true};
                BackgroundWorker::ProgressHandler OnProgress{};
                BackgroundWorker::CompletedHandler OnCompleted{};
                std::shared_ptr<IExecutor> CallbackExecutor{}; // default InlineExecutor if null
            };

            using WorkerStartedHandler   = std::function<void(TaskManager&, TaskId)>;
            using WorkerProgressHandler  = std::function<void(TaskManager&, TaskId, int, const std::any&)>;
            using WorkerCompletedHandler = std::function<void(TaskManager&, TaskId, const std::any&, bool, std::exception_ptr)>;
            using AllCompletedHandler    = std::function<void(TaskManager&)>;

        public:
            static TaskManager& Instance(std::size_t maxParallel = std::thread::hardware_concurrency()) 
            {
                if(maxParallel <= 0)
                {
                    MOTION_CORE_WARN("Number of system threads returned 0, Falling back to default 4");
                    maxParallel = 4;
                }

                static TaskManager s_instance(maxParallel);
                return s_instance;
            }

            TaskManager(const TaskManager&) = delete;
            TaskManager& operator=(const TaskManager&) = delete;
            TaskManager(TaskManager&&) = delete;
            TaskManager& operator=(TaskManager&&) = delete;

            void SetOnWorkerStarted(WorkerStartedHandler h)       { std::scoped_lock lk(m_mutex); m_onWorkerStarted = std::move(h); }
            void SetOnWorkerProgress(WorkerProgressHandler h)     { std::scoped_lock lk(m_mutex); m_onWorkerProgress = std::move(h); }
            void SetOnWorkerCompleted(WorkerCompletedHandler h)   { std::scoped_lock lk(m_mutex); m_onWorkerCompleted = std::move(h); }
            void SetOnAllCompleted(AllCompletedHandler h)         { std::scoped_lock lk(m_mutex); m_onAllCompleted = std::move(h); }

            void SetMaxParallel(std::size_t n) 
            {
                if (n == 0) n = 1;
                std::scoped_lock lk(m_mutex);
                m_maxParallel = n;
                TryLaunchMore_();
            }

            std::size_t GetMaxParallel() const noexcept { return m_maxParallel.load(std::memory_order_acquire); }

            TaskId AddTask(TaskSpec spec) 
            {
                if (!spec.DoWork) throw std::logic_error("TaskSpec.DoWork is required");
                TaskId id = m_nextId.fetch_add(1, std::memory_order_acq_rel) + 1;

                auto rec = std::make_unique<Record>();
                rec->id = id;
                rec->status = TaskStatus::Pending;
                rec->spec = std::move(spec);

                {
                    std::scoped_lock lk(m_mutex);
                    m_records.emplace(id, std::move(rec));
                    m_pending.push_back(id);
                }
                
                TryLaunchMore_();
                return id;

            }

            bool StartNow(TaskId id) 
            {
                std::scoped_lock lk(m_mutex);
                if (!HasRecord_(id)) return false;
                auto& r = *m_records[id];
                if (r.status != TaskStatus::Pending) return false;
                auto it = std::find(m_pending.begin(), m_pending.end(), id);
                if (it != m_pending.end()) 
                {
                    m_pending.erase(it);
                    m_pending.push_front(id);
                }

                TryLaunchMore_();
                return true;
            }

            bool Cancel(TaskId id) 
            {
                {
                    std::scoped_lock lk(m_mutex);
                    auto it = m_records.find(id);
                    if (it == m_records.end()) return false;
                    auto& r = *it->second;

                    if (r.status == TaskStatus::Pending) 
                    {
                        auto qit = std::find(m_pending.begin(), m_pending.end(), id);
                        if (qit != m_pending.end()) m_pending.erase(qit);
                        r.status = TaskStatus::Cancelled;
                        FireCompletedNoWorker_(r, std::any{}, true, nullptr);
                        TryAllDone_();
                        m_cv.notify_all();
                        return true;
                    }

                    if (r.status == TaskStatus::Running && r.worker) 
                    {
                        r.worker->CancelAsync();
                        return true;
                    }

                    return false; // already finished
                }
            }

            void CancelAll() 
            {
                std::vector<TaskId> toCancel;
                {
                    std::scoped_lock lk(m_mutex);
                    for (TaskId id : m_pending) 
                    {
                        auto& r = *m_records[id];
                        r.status = TaskStatus::Cancelled;
                        FireCompletedNoWorker_(r, std::any{}, true, nullptr);
                    }
                    m_pending.clear();
                    for (TaskId id : m_running) toCancel.push_back(id);
                    m_cv.notify_all();
                }

                for (TaskId id : toCancel) 
                {
                    std::scoped_lock lk(m_mutex);
                    auto it = m_records.find(id);
                    if (it != m_records.end() && it->second->worker) it->second->worker->CancelAsync();
                }
            }

            struct Counters 
            {
                std::size_t pending{};
                std::size_t running{};
                std::size_t completed{};
                std::size_t cancelled{};
                std::size_t faulted{};
                std::size_t total{};
            };

            Counters GetCounters() const 
            {
                std::scoped_lock lk(m_mutex);
                Counters c{};
                c.pending = m_pending.size();
                c.running = m_running.size();
                for (auto& [id, rec] : m_records) 
                {
                    switch (rec->status) {
                        case TaskStatus::Completed: ++c.completed; break;
                        case TaskStatus::Cancelled: ++c.cancelled; break;
                        case TaskStatus::Faulted:   ++c.faulted;   break;
                        default: break;
                    }
                }
                c.total = m_records.size();
                return c;
            }

            std::optional<TaskStatus> GetStatus(TaskId id) const 
            {
                std::scoped_lock lk(m_mutex);
                auto it = m_records.find(id);
                if (it == m_records.end()) return std::nullopt;
                return it->second->status;
            }

            std::optional<int> GetLastPercent(TaskId id) const 
            {
                std::scoped_lock lk(m_mutex);
                auto it = m_records.find(id);
                if (it == m_records.end()) return std::nullopt;
                return it->second->lastPercent;
            }

            std::optional<std::any> GetResult(TaskId id) const 
            {
                std::scoped_lock lk(m_mutex);
                auto it = m_records.find(id);
                if (it == m_records.end()) return std::nullopt;
                if (!it->second->result) return std::nullopt;
                return *it->second->result;
            }

            int GetOverallPercent() const 
            {
                std::scoped_lock lk(m_mutex);
                std::size_t n = m_records.size();
                if (n == 0) return 100;
                std::size_t sum = 0;
                for (auto& [id, rec] : m_records) 
                {
                    if (rec->status == TaskStatus::Completed ||
                        rec->status == TaskStatus::Cancelled ||
                        rec->status == TaskStatus::Faulted) sum += 100;
                    else sum += static_cast<std::size_t>(rec->lastPercent);
                }
                return static_cast<int>(sum / n);
            }

            void WaitIdle() 
            {
                std::unique_lock lk(m_mutex);
                m_cv.wait(lk, [this] { return m_pending.empty() && m_running.empty(); });
            }

            void Shutdown() 
            {
                CancelAll();
                WaitIdle();
                
                // Optionally clear out finished records to free memory:
                m_records.clear();
            }

        private:
            struct Record 
            {
                TaskId id{};
                TaskStatus status{TaskStatus::Pending};
                TaskSpec spec{};
                std::unique_ptr<BackgroundWorker> worker;
                int lastPercent{0};
                std::unique_ptr<std::any> lastUserState;
                std::unique_ptr<std::any> result;
                std::exception_ptr error{};
                bool cancelled{false};
            };

        private:
            explicit TaskManager(std::size_t maxParallel)
                : m_maxParallel(maxParallel == 0 ? 1 : maxParallel) {}

            bool HasRecord_(TaskId id) const { return m_records.find(id) != m_records.end(); }

            void TryLaunchMore_() 
            {
                while (m_running.size() < m_maxParallel.load(std::memory_order_acquire) && !m_pending.empty()) 
                {
                    TaskId id = m_pending.front();
                    m_pending.pop_front();
                    auto it = m_records.find(id);
                    if (it == m_records.end()) continue;
                    auto& rec = *it->second;
                    if (rec.status != TaskStatus::Pending) continue;
                    Launch_(rec);
                }
            }

            void Launch_(Record& rec) 
            {
                rec.worker = std::make_unique<BackgroundWorker>();
                rec.worker->WorkerReportsProgress = rec.spec.WorkerReportsProgress;
                rec.worker->WorkerSupportsCancellation = rec.spec.WorkerSupportsCancellation;

                if (rec.spec.CallbackExecutor) 
                {
                    rec.worker->SetCallbackExecutor(rec.spec.CallbackExecutor);
                } 
                else 
                {
                    // Run progress/completed on the main thread by default
                    rec.worker->SetCallbackExecutor(MainThreadDispatcher::Shared());
                }

                rec.worker->SetDoWork(rec.spec.DoWork);

                // Wire progress
                rec.worker->SetProgressChanged([this, tid = rec.id](BackgroundWorker&, int percent, const std::any& userState) 
                {
                    BackgroundWorker::ProgressHandler taskPh;
                    WorkerProgressHandler mgrPh;
                    {
                        std::scoped_lock lk(m_mutex);
                        auto it = m_records.find(tid);
                        if (it == m_records.end()) return;
                        it->second->lastPercent = percent;
                        it->second->lastUserState = std::make_unique<std::any>(userState);
                        taskPh = it->second->spec.OnProgress;
                        mgrPh  = m_onWorkerProgress;
                    }
                    if (taskPh) { try { taskPh(*m_records[tid]->worker, percent, userState); } catch (...) {} }
                    if (mgrPh)  { try { mgrPh(*this, tid, percent, userState); } catch (...) {} }
                });

                // Wire completion
                rec.worker->SetRunWorkerCompleted([this, tid = rec.id](BackgroundWorker&, const std::any& result, bool cancelled, std::exception_ptr error) 
                {
                    BackgroundWorker::CompletedHandler taskCh;
                    WorkerCompletedHandler mgrCh;
                    {
                        std::scoped_lock lk(m_mutex);
                        auto it = m_records.find(tid);
                        if (it == m_records.end()) { m_cv.notify_all(); return; }
                        auto& r = *it->second;
                        r.cancelled = cancelled;
                        r.error = error;
                        if (result.has_value()) r.result = std::make_unique<std::any>(result);

                        if (error) r.status = TaskStatus::Faulted;
                        else if (cancelled) r.status = TaskStatus::Cancelled;
                        else r.status = TaskStatus::Completed;

                        m_running.erase(tid);
                        taskCh = r.spec.OnCompleted;
                        mgrCh  = m_onWorkerCompleted;
                    }

                    if (taskCh) { try { taskCh(*m_records[tid]->worker, result, cancelled, error); } catch (...) {} }
                    if (mgrCh)  { try { mgrCh(*this, tid, result, cancelled, error); } catch (...) {} }

                    TryLaunchMore_();
                    TryAllDone_();
                    m_cv.notify_all();
                });

                // Mark running + notify started (outside lock)
                WorkerStartedHandler startedCb;
                {
                    std::scoped_lock lk(m_mutex);
                    m_running.insert(rec.id);
                    rec.status = TaskStatus::Running;
                    startedCb = m_onWorkerStarted;
                }
                if (startedCb) { try { startedCb(*this, rec.id); } catch (...) {} }

                rec.worker->RunWorkerAsync(rec.spec.Argument);
            }

            void FireCompletedNoWorker_(Record& rec, const std::any& result, bool cancelled, std::exception_ptr err) 
            {
                rec.cancelled = cancelled;
                rec.error = err;
                if (result.has_value()) rec.result = std::make_unique<std::any>(result);
                if (err) rec.status = TaskStatus::Faulted;
                else if (cancelled) rec.status = TaskStatus::Cancelled;
                else rec.status = TaskStatus::Completed;

                auto mgrCh = m_onWorkerCompleted;
                if (mgrCh) { try { mgrCh(*this, rec.id, result, cancelled, err); } catch (...) {} }
            }

            void TryAllDone_() 
            {
                if (!m_onAllCompleted) return;
                if (!m_pending.empty() || !m_running.empty()) return;
                auto cb = m_onAllCompleted;
                try { cb(*this); } catch (...) {}
            }

        private:
            mutable std::mutex m_mutex;
            std::condition_variable m_cv;

            std::unordered_map<TaskId, std::unique_ptr<Record>> m_records;
            std::deque<TaskId> m_pending;
            std::unordered_set<TaskId> m_running;

            std::atomic<std::size_t> m_maxParallel{1};
            std::atomic<TaskId> m_nextId{0};

            WorkerStartedHandler   m_onWorkerStarted{};
            WorkerProgressHandler  m_onWorkerProgress{};
            WorkerCompletedHandler m_onWorkerCompleted{};
            AllCompletedHandler    m_onAllCompleted{};
    };
}


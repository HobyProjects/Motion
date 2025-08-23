#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <chrono>

namespace Motion 
{

    // --------------------------- Cancellation error ------------------------------
    struct TaskCanceled : std::exception {
        const char* what() const noexcept override { return "Task canceled"; }
    };

    // --------------------------- Progress ----------------------------------------
    struct TaskProgress 
    {
        std::atomic<std::uint64_t> current{0};
        std::atomic<std::uint64_t> total{0};
        std::atomic<bool>          started{false};
        std::atomic<bool>          done{false};
        std::atomic<bool>          cancel{false}; // cooperative cancel flag

        double fraction() const {
            const auto t = total.load(std::memory_order_relaxed);
            const auto c = current.load(std::memory_order_relaxed);
            if (t == 0) return done.load(std::memory_order_relaxed) ? 1.0 : 0.0;
            return static_cast<double>(c) / static_cast<double>(t);
        }
        int percent() const { return static_cast<int>(fraction() * 100.0 + 0.5); }
    };

    struct ProgressToken 
    {
        TaskProgress* p{};
        void SetTotal(std::uint64_t t)      const { if (p) p->total.store(t, std::memory_order_relaxed); }
        void Advance(std::uint64_t inc = 1) const { if (p) p->current.fetch_add(inc, std::memory_order_relaxed); }
        void SetCurrent(std::uint64_t v)    const { if (p) p->current.store(v, std::memory_order_relaxed); }
        bool IsCancelRequested()            const { return p && p->cancel.load(std::memory_order_relaxed); }
    };

    // --------------------------- Result wrapper ----------------------------------
    template<class R>
    struct TaskResult 
    {
        bool ok{false};
        std::optional<R> value;     // set when ok==true
        std::exception_ptr error;   // set when ok==false
    };
    template<>
    struct TaskResult<void> 
    {
        bool ok{false};
        std::exception_ptr error;
    };

    // --------------------------- Introspection -----------------------------------
    using TaskID = std::uint64_t;

    struct TaskInfo 
    {
        TaskID      id{};
        std::string name;
        int         percent{0};
        bool        started{false};
        bool        done{false};
    };

    struct WorkerInfo 
    {
        std::thread::id            tid{};
        std::atomic<std::uint64_t> tasks_processed{0};
        std::atomic<TaskID>        running_task_id{0}; // 0 = idle

        WorkerInfo() = default;
        WorkerInfo(const WorkerInfo&) = delete;
        WorkerInfo& operator=(const WorkerInfo&) = delete;
        WorkerInfo(WorkerInfo&& o) noexcept
            : tid(o.tid)
            , tasks_processed(o.tasks_processed.load(std::memory_order_relaxed))
            , running_task_id(o.running_task_id.load(std::memory_order_relaxed)) {}
        WorkerInfo& operator=(WorkerInfo&& o) noexcept {
            if (this != &o) {
                tid = o.tid;
                tasks_processed.store(o.tasks_processed.load(std::memory_order_relaxed), std::memory_order_relaxed);
                running_task_id.store(o.running_task_id.load(std::memory_order_relaxed), std::memory_order_relaxed);
            }
            return *this;
        }
    };

    struct PoolSnapshot 
    {
        std::size_t worker_count{};
        std::size_t queue_size{};
        std::uint64_t tasks_enqueued{};
        std::uint64_t tasks_completed{};
        std::vector<std::pair<std::thread::id, TaskID>> worker_running_task; // (tid, task_id)
    };

    // --------------------------- Task core ---------------------------------------
    struct TaskBase 
    {
        virtual ~TaskBase() = default;
        virtual void run(WorkerInfo* w) = 0;

        TaskID                        id{};
        std::string                   name;
        std::shared_ptr<TaskProgress> prog{std::make_shared<TaskProgress>()};
        std::function<void()>         on_done; // posts main-thread completion
    };

    template<class Fn>
    struct TaskImpl;

    template<class Fn>
    struct TaskImpl : TaskBase 
    {
        using R = std::invoke_result_t<Fn, ProgressToken>;
        Fn fn;
        std::promise<R> prom;

        explicit TaskImpl(Fn&& f) : fn(std::forward<Fn>(f)) {}

        void run(WorkerInfo* w) override {
            prog->started.store(true, std::memory_order_relaxed);
            try {
                if constexpr (std::is_void_v<R>) {
                    std::invoke(fn, ProgressToken{ prog.get() });
                    prom.set_value();
                } else {
                    auto r = std::invoke(fn, ProgressToken{ prog.get() });
                    prom.set_value(std::move(r));
                }
            } catch (...) {
                prom.set_exception(std::current_exception());
            }
            prog->done.store(true, std::memory_order_relaxed);
            if (on_done) on_done();
            if (w) {
                w->running_task_id.store(0, std::memory_order_relaxed);
                w->tasks_processed.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    // ---------------------- Main-thread dispatch (built-in) ----------------------
    namespace ThreadDispatch
    {
        inline std::mutex& main_mx() { static std::mutex m; return m; }
        inline std::deque<std::function<void()>>& main_q() { static std::deque<std::function<void()>> q; return q; }
        inline void post_main(std::function<void()> fn) 
        {
            auto& mx = main_mx(); auto& q = main_q();
            std::scoped_lock lk(mx); q.emplace_back(std::move(fn));
        }

        inline void pump_main() 
        {
            auto& mx = main_mx(); auto& q = main_q();
            std::deque<std::function<void()>> local;
            { std::scoped_lock lk(mx); local.swap(q); }
            for (auto& f : local) f();
        }
    }

    // --------------------------- ThreadPool --------------------------------------
    class ThreadPool 
    {
        public:
            explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency())
                : stop_(false) {
                if (threads == 0) threads = 1;
                workers_.resize(threads);
                threads_.reserve(threads);
                for (std::size_t i = 0; i < threads; ++i) {
                    threads_.emplace_back([this, i] {
                        WorkerInfo* w = &workers_[i];
                        w->tid = std::this_thread::get_id();
                        loop(w);
                    });
                }
            }
            ~ThreadPool() { shutdown(); }

            ThreadPool(const ThreadPool&) = delete;
            ThreadPool& operator=(const ThreadPool&) = delete;

            // ---- Submit with REQUIRED callback (two friendly overloads) ----
            // priority: higher value runs earlier; FIFO within same priority
            template<class Fn, class Cb>
            TaskID Submit(std::string name, Fn&& fn, Cb&& on_complete, int priority = 0) {
                return submit_impl(std::move(name), std::forward<Fn>(fn), std::forward<Cb>(on_complete), priority);
            }
            template<class Fn, class Cb>
            TaskID Submit(Fn&& fn, std::string name, Cb&& on_complete, int priority = 0) {
                return submit_impl(std::move(name), std::forward<Fn>(fn), std::forward<Cb>(on_complete), priority);
            }

            // ---- Introspection / progress queries ----
            PoolSnapshot Snapshot() const {
                PoolSnapshot s;
                s.worker_count   = workers_.size();
                { std::scoped_lock lk(mx_); s.queue_size = queue_.size(); }
                s.tasks_enqueued = tasks_enqueued_.load(std::memory_order_relaxed);
                s.tasks_completed= tasks_completed_.load(std::memory_order_relaxed);
                for (auto const& w : workers_) {
                    s.worker_running_task.emplace_back(w.tid, w.running_task_id.load(std::memory_order_relaxed));
                }
                return s;
            }

            std::optional<TaskInfo> GetTaskInfo(TaskID id) const {
                std::scoped_lock lk(reg_mx_);
                auto it = registry_.find(id);
                if (it == registry_.end()) return std::nullopt;
                auto& rec = it->second;
                TaskInfo ti;
                ti.id = id;
                ti.name = rec.name;
                ti.started = rec.prog->started.load(std::memory_order_relaxed);
                ti.done = rec.prog->done.load(std::memory_order_relaxed);
                ti.percent = rec.prog->percent();
                return ti;
            }

            std::vector<TaskID> FindByName(std::string_view name_prefix) const {
                std::vector<TaskID> out;
                std::scoped_lock lk(reg_mx_);
                out.reserve(registry_.size());
                for (auto const& [id, rec] : registry_) {
                    if (rec.name.rfind(name_prefix.data(), 0) == 0) out.push_back(id);
                }
                return out;
            }

            void PruneFinished(std::size_t keep_recent = 128) {
                std::scoped_lock lk(reg_mx_);
                std::vector<TaskID> done_ids;
                done_ids.reserve(registry_.size());
                for (auto const& [id, rec] : registry_) {
                    if (rec.prog->done.load(std::memory_order_relaxed)) done_ids.push_back(id);
                }
                if (done_ids.size() <= keep_recent) return;
                std::sort(done_ids.begin(), done_ids.end());
                const auto remove_count = done_ids.size() - keep_recent;
                for (size_t i = 0; i < remove_count; ++i) registry_.erase(done_ids[i]);
            }

            // ---- Cancellation ----
            bool Cancel(TaskID id) {
                std::scoped_lock lk(reg_mx_);
                auto it = registry_.find(id);
                if (it == registry_.end()) return false;
                it->second.prog->cancel.store(true, std::memory_order_relaxed);
                return true;
            }
            void CancelAll() {
                std::scoped_lock lk(reg_mx_);
                for (auto& [_, rec] : registry_) rec.prog->cancel.store(true, std::memory_order_relaxed);
            }

            void shutdown() {
                bool expected = false;
                if (!stop_.compare_exchange_strong(expected, true)) return; // already stopped
                cv_.notify_all();
                for (auto& t : threads_) if (t.joinable()) t.join();
                threads_.clear();
            }

        private:
            // Registry record
            struct Rec {
                std::string name;
                std::shared_ptr<TaskProgress> prog;
            };

            // Priority queue item
            struct QItem {
                int prio;                               // higher first
                std::uint64_t seq;                      // FIFO within same priority
                std::shared_ptr<TaskBase> task;
            };
            struct QLess {
                bool operator()(QItem const& a, QItem const& b) const noexcept {
                    if (a.prio != b.prio) return a.prio < b.prio;   // max-heap by priority
                    return a.seq > b.seq;                            // min-heap by seq
                }
            };

            template<class Fn, class Cb>
            TaskID submit_impl(std::string name, Fn&& fn, Cb&& on_complete, int priority) {
                using R = std::invoke_result_t<Fn, ProgressToken>;
                using WrapFn = std::function<R(ProgressToken)>;
                using OnCbSig =
                    std::conditional_t<std::is_void_v<R>,
                        std::function<void(TaskResult<void>&&, const TaskInfo&)>,
                        std::function<void(TaskResult<R>&&,   const TaskInfo&)>>;

                auto task = std::make_shared<TaskImpl<WrapFn>>(WrapFn(std::forward<Fn>(fn)));
                task->id   = next_id_.fetch_add(1, std::memory_order_relaxed);
                task->name = std::move(name);

                // Capture everything you'll need *before* moving `task`
                TaskID id          = task->id;                         // <-- store id
                std::string nm     = task->name;                       // (optional) if you need a copy
                auto weak_prog     = std::weak_ptr<TaskProgress>(task->prog);
                std::shared_future<R> sf = task->prom.get_future().share();

                {   // registry record for progress
                    std::scoped_lock lk(reg_mx_);
                    registry_[id] = Rec{ nm, task->prog };
                }

                // Build main-thread callback (uses captured id/nm/weak_prog/sf)
                if constexpr (std::is_void_v<R>) 
                {
                    OnCbSig cb(std::forward<Cb>(on_complete));
                    task->on_done = 
                    [sf, cb = std::move(cb), id, nm, weak_prog]() mutable 
                    {
                        ThreadDispatch::post_main([sf, cb = std::move(cb), id, nm, weak_prog]() mutable 
                        {
                            TaskInfo info{ id, nm, 0, true, true };
                            if (auto sp = weak_prog.lock()) { info.percent = sp->percent(); info.done = sp->done.load(); info.started = sp->started.load(); }
                            TaskResult<void> tr;
                            try { sf.get(); tr.ok = true; }
                            catch (...) { tr.ok = false; tr.error = std::current_exception(); }
                            cb(std::move(tr), info);

                        });
                    };

                } else 
                {
                    OnCbSig cb(std::forward<Cb>(on_complete));
                    task->on_done = 
                    [sf, cb = std::move(cb), id, nm, weak_prog]() mutable 
                    {
                        ThreadDispatch::post_main([sf, cb = std::move(cb), id, nm, weak_prog]() mutable {
                            TaskInfo info{ id, nm, 0, true, true };
                            if (auto sp = weak_prog.lock()) { info.percent = sp->percent(); info.done = sp->done.load(); info.started = sp->started.load(); }
                            TaskResult<R> tr;
                            try { tr.value = sf.get(); tr.ok = true; }
                            catch (...) { tr.ok = false; tr.error = std::current_exception(); }
                            cb(std::move(tr), info);
                        });
                    };
                }

                // Enqueue (this MOVE invalidates `task`, which is fine now)
                {
                    std::scoped_lock lk(mx_);
                    queue_.push(QItem{ priority, seq_counter_.fetch_add(1, std::memory_order_relaxed), std::move(task) });
                    tasks_enqueued_.fetch_add(1, std::memory_order_relaxed);
                }
                cv_.notify_one();

                return id;  // <-- return the stored id, not task->id
            }

            void loop(WorkerInfo* w) {
                while (true) {
                    std::shared_ptr<TaskBase> task;
                    {
                        std::unique_lock lk(mx_);
                        cv_.wait(lk, [&]{ return stop_.load(std::memory_order_relaxed) || !queue_.empty(); });
                        if (stop_.load(std::memory_order_relaxed) && queue_.empty()) break;

                        auto qi = queue_.top();
                        queue_.pop();
                        task = std::move(qi.task);
                    }
                    w->running_task_id.store(task->id, std::memory_order_relaxed);
                    task->run(w);
                    tasks_completed_.fetch_add(1, std::memory_order_relaxed);
                }
            }

        private:
            // Work queue (priority)
            mutable std::mutex                                mx_;
            std::condition_variable                           cv_;
            std::priority_queue<QItem, std::vector<QItem>, QLess> queue_;
            std::atomic<std::uint64_t>                        seq_counter_{0};

            // Threads & workers
            std::vector<std::thread>                          threads_;
            std::vector<WorkerInfo>                           workers_;

            // Registry (id -> name/progress) for external queries
            mutable std::mutex                                reg_mx_;
            std::unordered_map<TaskID, Rec>                   registry_;

            // State
            std::atomic<bool>          stop_;
            std::atomic<TaskID>        next_id_{1};
            std::atomic<std::uint64_t> tasks_enqueued_{0};
            std::atomic<std::uint64_t> tasks_completed_{0};
    };

    // --------------------------- Static facade -----------------------------------
    namespace Threads 
    {

        inline std::mutex& _mx() { static std::mutex m; return m; }
        inline std::unique_ptr<ThreadPool>& _instance() { static std::unique_ptr<ThreadPool> p; return p; }

        inline void Init(std::size_t threads = std::thread::hardware_concurrency()) {
            std::scoped_lock lk(_mx());
            if (_instance()) return;
            _instance() = std::make_unique<ThreadPool>(threads ? threads : 1);
        }

        // Shutdown
        inline void Shutdown() {
            std::unique_ptr<ThreadPool> tmp;
            { std::scoped_lock lk(_mx()); tmp = std::move(_instance()); }
            if (tmp) tmp->shutdown();
        }

        inline ThreadPool& Get() {
            {
                std::scoped_lock lk(_mx());
                if (!_instance()) {
                    _instance() = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
                    static bool hooked = (std::atexit([]{ Shutdown(); }), true);
                    (void)hooked;
                }
            }
            return *_instance();
        }

        // Submit overloads (REQUIRED callback). priority defaults to 0.
        template<class Fn, class Cb>
        TaskID Submit(std::string name, Fn&& fn, Cb&& on_complete, int priority = 0) 
        {
            return Get().Submit(std::move(name), std::forward<Fn>(fn), std::forward<Cb>(on_complete), priority);
        }
        template<class Fn, class Cb>
        TaskID Submit(Fn&& fn, std::string name, Cb&& on_complete, int priority = 0) 
        {
            return Get().Submit(std::forward<Fn>(fn), std::move(name), std::forward<Cb>(on_complete), priority);
        }

        // Progress / search / maintenance
        inline PoolSnapshot Snapshot()                        { return Get().Snapshot(); }
        inline std::optional<TaskInfo> Info(TaskID id)        { return Get().GetTaskInfo(id); }
        inline std::vector<TaskID>     Find(std::string_view name_prefix) { return Get().FindByName(name_prefix); }
        inline void                    PruneFinished(std::size_t keep_recent = 128) { Get().PruneFinished(keep_recent); }

        // Cancellation
        inline bool Cancel(TaskID id) { return Get().Cancel(id); }
        inline void CancelAll()       { Get().CancelAll(); }

        // Drive main-thread callbacks once per frame
        inline void PumpMain() { ThreadDispatch::pump_main(); }

    } // namespace Threads

} // namespace Motion
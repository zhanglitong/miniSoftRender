#pragma     once
#include    "FETask.hpp"
#include    <atomic>
#include    <condition_variable>
#include    <deque>
#include    <mutex>
#include    <thread>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 任务系统:
    ///   1. addTask 后由工作线程执行 FETask::exec()
    ///   2. 执行完成的任务进入完成队列
    ///   3. 主线程每帧调用 pump(),在完成队列所属线程(主线程)触发 FETask::finish()
    /// 注意:pump() 只能在主线程调用;工作线程数量默认 1。
    /// </summary>
    class   FETaskMgr
    {
    public:
        explicit    FETaskMgr(uint workerCount = 1)
        {
            _running.store(true);
            const   uint    count   =   (workerCount == 0) ? 1 : workerCount;
            for (uint i = 0; i < count; ++i)
                _workers.emplace_back([this]() { workerLoop(); });
        }
        ~FETaskMgr()
        {
            stop();
        }
        FETaskMgr(const FETaskMgr&) = delete;
        FETaskMgr&  operator = (const FETaskMgr&) = delete;
    public:
        /// <summary>
        /// 加入任务(线程安全,可在任意线程调用)
        /// </summary>
        void    addTask(Task task)
        {
            if (task == nullptr)
                return;
            {
                std::lock_guard<std::mutex>    lock(_mutex);
                if (!_running.load())
                {
                    task->cancel();
                    return;
                }
                _pending.push_back(task);
            }
            _cv.notify_one();
        }
        /// <summary>
        /// 主线程:取出已完成任务并触发完成回调
        /// </summary>
        /// <returns>本次处理的任务数量</returns>
        size_t  pump()
        {
            std::deque<Task>    finished;
            {
                std::lock_guard<std::mutex>    lock(_mutex);
                finished.swap(_finished);
            }
            for (auto& task : finished)
            {
                if (task != nullptr && !task->isCanceled())
                    task->finish();
            }
            return  finished.size();
        }
        /// <summary>
        /// 取消所有未执行以及已完成但尚未派发的任务(不触发完成回调)
        /// </summary>
        void    cancelAll()
        {
            std::deque<Task>    tasks;
            {
                std::lock_guard<std::mutex>    lock(_mutex);
                tasks.swap(_pending);
                tasks.insert(tasks.end(),_finished.begin(),_finished.end());
                _finished.clear();
            }
            for (auto& task : tasks)
            {
                if (task != nullptr)
                    task->cancel();
            }
        }
        /// <summary>
        /// 停止工作线程(等待正在执行的任务返回),退出前必须先调用
        /// </summary>
        void    stop()
        {
            if (!_running.exchange(false))
                return;
            _cv.notify_all();
            for (auto& worker : _workers)
            {
                if (worker.joinable())
                    worker.join();
            }
            _workers.clear();
            cancelAll();
        }
        inline  bool    running() const
        {
            return  _running.load();
        }
    protected:
        void    workerLoop()
        {
            for (;;)
            {
                Task    task;
                {
                    std::unique_lock<std::mutex>    lock(_mutex);
                    _cv.wait(lock,[this]()
                    {
                        return  !_running.load() || !_pending.empty();
                    });
                    if (!_running.load())
                        return;
                    task    =   _pending.front();
                    _pending.pop_front();
                }
                if (task == nullptr || task->isCanceled())
                    continue;
                bool    done    =   false;
                try
                {
                    done    =   task->exec();
                }
                catch (...)
                {
                    /// 执行体异常不能杀掉工作线程,统一按失败处理
                    task->setCode(-1);
                    done    =   true;
                }
                {
                    std::lock_guard<std::mutex>    lock(_mutex);
                    /// 停止过程中完成的任务不再派发,直接释放
                    if (!_running.load())
                        continue;
                    if (done)
                        _finished.push_back(task);
                    else
                        _pending.push_back(task);
                }
            }
        }
    protected:
        std::mutex                  _mutex;
        std::condition_variable     _cv;
        std::deque<Task>            _pending;
        std::deque<Task>            _finished;
        std::vector<std::thread>    _workers;
        std::atomic<bool>           _running{false};
    };
}

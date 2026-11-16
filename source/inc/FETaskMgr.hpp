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
    ///   2. exec() 返回 false 表示尚未完成,任务重新入队,由工作线程继续调度
    ///   3. exec() 返回 true 后由当前工作线程立即触发 FETask::finish()
    /// 注意:finish() 在工作线程执行,不是主线程。回调里需要访问主线程独占状态
    ///       (场景树/UI/渲染)时,用 FEScene::sendRunable 投递到场景主线程执行;
    ///       若直接访问则必须自行持 FEDevice::gpuMutex 与帧循环互斥。
    ///       任务对象的最后一个引用也可能在工作线程释放。
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
        /// 取消所有未执行的任务(不触发完成回调),正在执行的任务只能靠 isCanceled() 提前退出
        /// </summary>
        void    cancelAll()
        {
            std::deque<Task>    tasks;
            {
                std::lock_guard<std::mutex>    lock(_mutex);
                tasks.swap(_pending);
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
        bool    running() const
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
                if (!done)
                {
                    std::lock_guard<std::mutex>    lock(_mutex);
                    /// 停止过程中未完成的任务不再调度
                    if (!_running.load())
                        continue;
                    _pending.push_back(task);
                    continue;
                }
                /// 停止过程中完成的任务不再派发,直接释放
                if (!_running.load() || task->isCanceled())
                    continue;
                try
                {
                    task->finish();
                }
                catch (...)
                {
                    /// 完成回调异常同样不能杀掉工作线程
                    task->setCode(-1);
                }
            }
        }
    protected:
        std::mutex                  _mutex;
        std::condition_variable     _cv;
        std::deque<Task>            _pending;
        std::vector<std::thread>    _workers;
        std::atomic<bool>           _running{false};
    };
}

#pragma     once
#include    <atomic>
#include    <functional>
#include    "FEMath.hpp"
#include    "FEObject.h"
#include    "FEProgress.hpp"
#include    "FEBuffer.hpp"
#include    "FEReaderHelper.hpp"
#include    "FEWriterHelper.hpp"
namespace   FE
{
    enum TaskState : uint32_t
    {
        TS_Waiting,
        TS_Run,
        TS_Pause,
        TS_Finished,
        TS_Max,
    };
    /// <summary>
    /// 内置变量
    /// </summary>
    constexpr   const char* TBCXSerialize    =   "CTXSerialize";
    /// <summary>
    /// Task Buildin Param
    /// 内置参数
    /// </summary>
    constexpr   const char* TBWriteFile      =   "TBWriteFile";
    constexpr   const char* TBWriteBuffer    =   "TBWriteBuffer";
    /// <summary>
    /// Task Buildin Param
    /// 内置参数
    /// </summary>
    constexpr   const char* TBReadFiles     =   "TBReadFiles";
    constexpr   const char* TBReadBuffers   =   "TBReadBuffers";

    using   TaskParam   =   std::variant<std::monostate,bool,int16_t,uint16_t,uint32_t,int32_t,uint64_t,int64_t,float,double
                                        ,String,Strings
                                        ,Object,Objects,Buffer,Buffers
                                        ,FESerializeCtx*
                                        ,FEWriterPtr
                                        >;
    using   TaskParams  =   std::map<String,TaskParam>;


    class   FETask :public FEProgress
    {
    public:
        /// <summary>
        /// 执行体(工作线程调用)
        /// </summary>
        /// <returns>true 表示任务已完成,false 表示未完成,任务系统会继续调度</returns>
        using   ExecFunc    =   std::function<bool(FETask&)>;
        /// <summary>
        /// 完成回调(由任务系统在完成队列所属线程调用,即主线程)
        /// </summary>
        using   FinishFunc  =   std::function<void(FETask&)>;
    protected:
        /// <summary>
        /// 任务状态:工作线程与主线程都会访问,使用原子变量
        /// </summary>
        std::atomic<TaskState>  _state{TS_Waiting};
        /// <summary>
        /// 取消标记:外部请求取消后,执行体可通过 isCanceled() 提前退出
        /// </summary>
        std::atomic<bool>       _canceled{false};
        /// <summary>
        /// 如果是reader,则是结果
        /// 如果是writer,则是被操作对象
        /// </summary>
        Objects     _objects;
        /// <summary>
        /// _code >= 0 ,成功, _code < 0 失败
        /// </summary>
        int64       _code   =   0;
        /// <summary>
        /// 正常情况下
        /// </summary>
        TaskParams  _param;
        /// <summary>
        /// 任务名称，方便调试使用,无实际业务价值
        /// </summary>
        String      _name;
        /// <summary>
        /// 自定义执行体(工作线程)
        /// </summary>
        ExecFunc    _execFunc;
        /// <summary>
        /// 自定义完成回调(主线程)
        /// </summary>
        FinishFunc  _finishFunc;
    public:
        FETask(FEContext& ctx)
            :FEProgress(ctx)
        {
            char    szDefault[32]   =   {0};
            sprintf(szDefault,"Task[%p]",this);
            _name   =   szDefault;
        }
        FETask(const FETask& other)
            :FEProgress(other)
            ,_state(other._state.load())
            ,_canceled(other._canceled.load())
            ,_execFunc(other._execFunc)
            ,_finishFunc(other._finishFunc)
        {
            char    szDefault[32]   =   {0};
            sprintf(szDefault,"Task[%p]",this);
            _name   =   szDefault;
        }
        const   String& name() const
        {
            return  _name;
        }
        inline  auto&   setName(const String& name)
        {
            _name   =   name;
            return  *this;
        }
        /// <summary>
        /// 设置执行体(工作线程中执行)
        /// </summary>
        inline  auto&   setExec(ExecFunc func)
        {
            _execFunc   =   std::move(func);
            return  *this;
        }
        /// <summary>
        /// 设置完成回调(主线程中执行)
        /// </summary>
        inline  auto&   setFinish(FinishFunc func)
        {
            _finishFunc =   std::move(func);
            return  *this;
        }
        /// <summary>
        /// 任务是否已完成
        /// </summary>
        inline  bool    isFinished() const
        {
            return  _state.load() == TS_Finished;
        }
        /// <summary>
        /// 是否已被取消
        /// </summary>
        inline  bool    isCanceled() const
        {
            return  _canceled.load();
        }
        /// <summary>
        /// 结果码:>= 0 成功,< 0 失败
        /// </summary>
        inline  int64   code() const
        {
            return  _code;
        }
        inline  auto&   setCode(int64 code)
        {
            _code   =   code;
            return  *this;
        }
        // <summary>
        /// 任务执行体,系统会优先调用该函数执行任务，如果返回false
        /// 任务会调用  _evtRun;
        /// </summary>
        /// <returns>返回true,说明是已经执行了任务，如果false 系统会认为任务还没有执行,会调用任务管理继续执行任务</returns>
        inline  bool    exec()
        {
            _state.store(TS_Run);
            /// <summary>
            /// 1. 先调用开始
            /// </summary>
            onStart();
            /// 2. 执行
            return  onExec();
        }
        /// <summary>
        /// 取消通知
        /// </summary>
        /// <returns></returns>
        inline  bool    cancel()
        {
            _canceled.store(true);
            _state.store(TS_Finished);
            return  onCancel();
        }
        /// <summary>
        /// 任务完成
        /// </summary>
        inline  bool    finish()
        {
            _state.store(TS_Finished);
            return  onFinish();
        }
        /// <summary>
        /// 读取结果或者是被写入数据
        /// </summary>
        /// <returns></returns>
        const   Objects&objects() const
        {
            return  _objects;
        }
        /// <summary>
        /// 设置被写入对象数据
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        inline   auto&  setObjects(const Objects& objects)
        {
            _objects    =   objects;
            return  *this;
        }
        inline   auto&  setObjects(Objects&& objects)
        {
            _objects    =   std::move(objects);
            return  *this;
        }
        /// <summary>
        /// 设置参数
        /// </summary>
        /// <param name="name"></param>
        /// <param name="param"></param>
        /// <returns></returns>
        inline  auto&   setParam(const String& name,const TaskParam& param)
        {
            _param[name]    =   param;
            return  *this;
        }
        /// <summary>
        /// 获取参数
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        inline  auto    param(const String& name)
        {
            auto    itr =   _param.find(name);
            if (itr != _param.end())
                return  itr->second;
            else
                return  TaskParam();
        }
    protected:
        /// <summary>
        /// 通知开始执行，用来打印一些信息等
        /// </summary>
        virtual bool    onStart()
        {
            return  true;
        }
        // <summary>
        /// 任务执行体,系统会优先调用该函数执行任务，如果返回false
        /// 任务会调用  _evtRun;
        /// </summary>
        /// <returns>返回true,说明是已经执行了任务，如果false 系统会认为任务还没有执行,会调用任务管理继续执行任务</returns>
        virtual bool    onExec()
        {
            return  _execFunc ? _execFunc(*this) : false;
        }
        /// <summary>
        /// 任务取消通知
        /// </summary>
        /// <returns></returns>
        virtual bool    onCancel()
        {
            return  false;
        }
        /// <summary>
        /// 任务完成通知
        /// </summary>
        virtual bool    onFinish()
        {
            if (_finishFunc)
            {
                _finishFunc(*this);
                return  true;
            }
            return  false;
        }
    };


    using   Task    =   SharedPtr<FETask>;
    using   Tasks   =   FEVector<FETask>;
}

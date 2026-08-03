#pragma     once
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
    protected:
        TaskState   _state  =   TS_Waiting;
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
        // <summary>
        /// 任务执行体,系统会优先调用该函数执行任务，如果返回false
        /// 任务会调用  _evtRun;
        /// </summary>
        /// <returns>返回true,说明是已经执行了任务，如果false 系统会认为任务还没有执行,会调用任务管理继续执行任务</returns>
        inline  bool    exec()
        {
            _state  =   TS_Run;
            /// 1. 先调用开始
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
            _state  =   TS_Finished;
            return  onCancel();
        }
        /// <summary>
        /// 任务完成
        /// </summary>
        inline  bool    finish()
        {
            _state  =   TS_Finished;
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
            return  false;
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
            return  false;
        }
    };


    using   Task    =   SharedPtr<FETask>;
    using   Tasks   =   FEVector<FETask>;
}
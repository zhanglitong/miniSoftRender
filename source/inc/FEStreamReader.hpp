#pragma     once
#include    "FEMath.hpp"
#include    "FEObject.h"
#include    "FEBuffer.hpp"
#include    "FETask.hpp"
#include    "FEFileFormat.hpp"
#include    "FEContext.hpp"
#include    <filesystem>

namespace   FE
{
    class   FEStreamReader :public FETask
    {
    public: 
        using   Reader  =   SharedPtr<FEStreamReader>;
    public:
        FEStreamReader(FEContext& ctx)
            :FETask(ctx)
        {}
        FEStreamReader(const FEStreamReader& other)
            :FETask(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        /// <returns></returns>
        virtual Formats formats() const =   0;
        /// <summary>
        /// 接口调用加载文件
        /// </summary>
        /// <param name="file"></param>
        /// <returns></returns>
        virtual Objects readFiles(const Strings& files)  =   0;
        /// <summary>
        /// 输入多个buffer,从多个buffer中读取对象数据
        /// </summary>
        /// <param name="buffers"></param>
        /// <param name="params"></param>
        /// <returns></returns>
        virtual Objects readBuffers(const Buffers& buffers) =   0;
    protected:
        /// <summary>
        /// 通知开始执行，用来打印一些信息等
        /// </summary>
        virtual bool    onStart() override
        {
            LOG_DBG("onStart(%s)!",_name.c_str());
            return  true;
        }
        // <summary>
        /// 任务执行体,系统会优先调用该函数执行任务，如果返回false
        /// 任务会调用  _evtRun;
        /// </summary>
        /// <returns>返回true,说明是已经执行了任务，如果false 系统会认为任务还没有执行,会调用任务管理继续执行任务</returns>
        virtual bool    onExec() override
        {
            LOG_DBG("onExec(%s)!",_name.c_str());
            auto    fileParam       =   param(TBReadFiles);
            Objects results;
            if (!std::holds_alternative<std::monostate>(fileParam))
            {
                auto    fileNames   =   std::get<Strings>(fileParam);
                auto    temp        =   readFiles(fileNames);
                results.insert(results.end(), temp.begin(),temp.end());
            }
            auto    bufferParam =   param(TBReadBuffers);
            if (!std::holds_alternative<std::monostate>(bufferParam))
            {
                auto    buffers     =   std::get<Buffers>(bufferParam);
                auto    temp        =   readBuffers(buffers);
                results.insert(results.end(), temp.begin(),temp.end());
            }
            if (results.empty())
                return  false;
            else
                _objects    =   std::move(results);
            return  true;
        }
        /// <summary>
        /// 任务取消通知
        /// </summary>
        /// <returns></returns>
        virtual bool    onCancel() override
        {
            LOG_DBG("onCancel(%s)!",_name.c_str());
            return  true;
        }
        /// <summary>
        /// 任务完成通知
        /// </summary>
        virtual bool    onFinish() override
        {
            LOG_DBG("onFinish(%s)!",_name.c_str());
            return  true;
        }
    };
}

#pragma     once
#include    "FEMath.hpp"
#include    "FEObject.h"
#include    <atomic>
namespace   FE
{
    class   FEProgress :public FEObject
    {
    public:
        FEProgress(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEProgress(const FEProgress& other)
            :FEObject(other)
            ,_progress(other._progress.load())
        {}
        /// <summary>
        /// 获取进度
        /// </summary>
        /// <returns></returns>
        real    progress() const
        {
            return  _progress.load();
        }
        auto&   setProgress(real val)
        {
            const   real    clamped =   std::clamp(val,real(0.0),real(1.0));
            _progress.store(clamped);
            onProgress(clamped);
            return  *this;
        }
    protected:
        virtual void    onProgress(real val)
        {
            UNUSED(val);
        }
    protected:
        /// <summary>
        /// 原子进度:允许工作线程写入、主线程读取(进度条)
        /// </summary>
        std::atomic<real>   _progress{real(0)};
    };
}

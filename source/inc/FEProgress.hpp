#pragma     once
#include    "FEMath.hpp"
#include    "FEObject.h"
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
        {}
        /// <summary>
        /// 获取进度
        /// </summary>
        /// <returns></returns>
        real    progress() const
        {
            return  _progress;
        }
        auto&   setProgress(real val)
        {
            _progress   =   std::clamp(val,real(0.0),real(1.0));
            onProgress(_progress);
            return  *this;
        }
    protected:
        virtual void    onProgress(real val)
        {
            (void)val;
        }
    protected:
        real    _progress   =   0;
    };
}

#pragma     once

#include    "FERasterState.h"
#include    "../inc/graphic/FEPBuffer.h"


namespace   FE
{
    DEFINE_CLASS_UUID(FERaster,"{C47EFE5F-93E0-4CC6-846D-54BF42246088}");
    /// <summary>
    /// 该类负责传统光栅化绘制
    /// 接口可以被重写，例如GPU绘制，光线追踪等
    /// </summary>
    class   FE_API  FERaster :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FERaster)
    public:
        FERaster(FEContext& ctx);
        FERaster(const FERaster& other);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="ptr"></param>
        void        setPBuffer(PBufferPtr ptr)
        {
            _pbuffer    =   ptr;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        PBufferPtr  getPBuffer() 
        {
            return  _pbuffer;
        }
    protected:
        PBufferPtr  _pbuffer;
    };

    using   RasterPtr   =   SharedPtr<FERaster>;
}

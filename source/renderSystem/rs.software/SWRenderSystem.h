#pragma     once
#include    "graphic/FERenderSystem.h"
#include    "SWDevice.h"

namespace   FE
{
    DEFINE_CLASS_UUID(SWRenderSystem,"{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}");
    class   SWRenderSystem :public FERenderSystem
    {
        IMPLEMENT_CLASS_REFLECT(SWRenderSystem)
    public:
        SWRenderSystem(FEContext& ctx)
            :FERenderSystem(ctx)
        {}
        SWRenderSystem(const SWRenderSystem& other)
            :FERenderSystem(other)
        {}
        virtual ~SWRenderSystem();

        virtual bool    isValid() const
        {
            return  true;
        }
        virtual void    initialize()
        {}
        virtual void    destroy();
        virtual Handle  native()  const
        {
            return  Handle(nullptr);
        }
        virtual const GPUs& gpuList()   const   override
        {
            return  _gpus;
        }
        virtual FEResult    create()            override
        {
            return  FEResult::ER_SUCCESS;
        }
        virtual Device      createDevice()      override
        {
            return  new SWDevice(_ctx,*this);
        }
    protected:
        GPUs    _gpus;
    };
}

#pragma     once
#include    "graphic/FERenderSystem.h"
#include    "wgpu.h"


namespace   FE
{
    DEFINE_CLASS_UUID(WGRenderSystem,"{730F1C2C-7AE6-4A69-A39E-0C591C659E97}");
    class   WGRenderSystem :public FERenderSystem
    {
        IMPLEMENT_CLASS_REFLECT(WGRenderSystem)
    public:
        WGRenderSystem(FEContext& ctx);

        WGRenderSystem(const WGRenderSystem& other)
            :FERenderSystem(other)
        {}
        virtual ~WGRenderSystem();

public:
        inline  auto    instance() const
        {
            return  _native;
        }
        /// <summary>
        /// ???????????Ч??
        /// </summary>
        virtual bool    isValid() const
        {
            return  _native != nullptr;
        }
        /// <summary>
        /// ?????????????洴???????????????е????????????????
        /// ?????????????????????????????????????
        /// ?ú????????????????????
        /// </summary>
        virtual void    initialize()
        {}
        /// <summary>
        /// ???????????????????????????
        /// </summary>
        virtual void    destroy();
        /// <summary>
        /// ??????,?????????????
        /// </summary>
        virtual Handle  native()  const
        {
            return  Handle(_native);
        }
        /// <summary>
        /// ???????б?
        /// </summary>
        /// <returns></returns>
        virtual const GPUs&    gpuList()   const   override
        {
            return  _gpus;
        }
        /// <summary>
        /// ?????豸
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        virtual FEResult    create()            override;
        virtual Device      createDevice()      override;
    public:
        void            debug(WGPULogLevel level,const char* msg);
    protected:
        GPUs            gpuListImpl() const;

    protected:
        WGPUInstance    _native    =   nullptr;
        GPUs            _gpus;
    };
}

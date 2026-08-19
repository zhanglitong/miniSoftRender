#pragma     once

#include    "FERenderContext.h"
#include    "FEDevice.h"
#include    "FEShader.h"

namespace   FE
{

    enum   GPUType :uint8
    {
        DEV_TYPE_OTHER           = 0,
        DEV_TYPE_INTEGRATED_GPU  = 1,
        DEV_TYPE_DISCRETE_GPU    = 2,
        DEV_TYPE_VIRTUAL_GPU     = 3,
        DEV_TYPE_CPU             = 4,
    } ;

    struct  GPU
    {
        FEUuid      gpuId;
        String      name;
        GPUType     type        =   DEV_TYPE_OTHER;
        uint        apiVersion  =   0;
        Handle      gpu         =   nullptr;

        bool    isValid() const
        {
            return  gpu != nullptr;
        }
    };
    using   GPUs    =   std::vector<GPU>;

    class   FERenderSystem :public FEObject
    {
    public:
        using   RenderSys   =   SharedPtr<FERenderSystem>;
        using   Handle      =   void*;
    public:
        
        static  inline  auto    nameOf(GPUType type)
        {
            switch(type)
            {
            case DEV_TYPE_OTHER         :   return  "OTHER";
            case DEV_TYPE_INTEGRATED_GPU:   return  "INTEGRATED";
            case DEV_TYPE_DISCRETE_GPU  :   return  "DISCRETE";
            case DEV_TYPE_VIRTUAL_GPU   :   return  "VIRTUAL";
            case DEV_TYPE_CPU           :   return  "CPU";
            default                     :   return  "OTHER";              
            }
        }
        static  inline  auto    GPUTypeFromName(const char* name)
        {
            if(_stricmp(nameOf(DEV_TYPE_OTHER               ),  name) == 0)     return DEV_TYPE_OTHER           ;
            else if(_stricmp(nameOf(DEV_TYPE_INTEGRATED_GPU ),  name) == 0)     return DEV_TYPE_INTEGRATED_GPU  ;
            else if(_stricmp(nameOf(DEV_TYPE_DISCRETE_GPU   ),  name) == 0)     return DEV_TYPE_DISCRETE_GPU    ;
            else if(_stricmp(nameOf(DEV_TYPE_VIRTUAL_GPU    ),  name) == 0)     return DEV_TYPE_VIRTUAL_GPU     ;
            else if(_stricmp(nameOf(DEV_TYPE_CPU            ),  name) == 0)     return DEV_TYPE_CPU             ;
            else                                                                return DEV_TYPE_OTHER           ;
        }
    public:
        FERenderSystem(FEContext& ctx)
            :FEObject(ctx)
        {}
        FERenderSystem(const FERenderSystem& other)
            :FEObject(other)
        {}
        /// <summary>
        /// 获取信息
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        inline  GPU         gpu(const FEUuid& id)
        {   
            auto&   gpus    =   gpuList();
            for (auto& gpu : gpus)
            {
                if (gpu.gpuId == id)
                    return  gpu;
            }
            return  {};
        }
        /// <summary>
        /// 获取显卡列表
        /// </summary>
        /// <returns></returns>
        virtual const GPUs& gpuList() const         =   0;
        virtual FEResult    create()                =   0;
        virtual Device      createDevice()          =   0;
    public:
        /// <summary>
        /// 创建渲染系统
        /// </summary>
        /// <param name="ctx"></param>
        /// <param name="clsId"></param>
        /// <returns></returns>
        static  RenderSys   create(FEContext& ctx,const FEUuid& clsId)
        {
            auto    pCreater     =   ctx.creators().isExist(clsId);
            if (pCreater == nullptr)
                return  nullptr;
            auto    result  =   (*pCreater)(ctx,{});
            if (result == nullptr)
                return  nullptr;
            else
                return  result->cast<FERenderSystem>();
        }
    };

    using   RenderSys   =   SharedPtr<FERenderSystem>;
    /// <summary>
    ///  {6BA6DF09-E1D8-449B-893E-140952BD6BEE}
    /// </summary>
    inline  static  FEUuid  RS_VULKAN       =   FEUuid({ 0x6ba6df09, 0xe1d8, 0x449b, { 0x89, 0x3e, 0x14, 0x9, 0x52, 0xbd, 0x6b, 0xee } });
    /// <summary>
    /// {730F1C2C-7AE6-4A69-A39E-0C591C659E97}
    /// </summary>
    static const    FEUuid  RS_WEBGPU       =   FEUuid({ 0x730f1c2c, 0x7ae6, 0x4a69, { 0xa3, 0x9e, 0xc, 0x59, 0x1c, 0x65, 0x9e, 0x97 } });
    /// <summary>
    /// {01B71260-C1CE-4DF4-B58C-7E727DFDCBB4}
    /// </summary>
    inline  static  FEUuid  RS_FASTEARTH    =   FEUuid({ 0x1b71260, 0xc1ce, 0x4df4, { 0xb5, 0x8c, 0x7e, 0x72, 0x7d, 0xfd, 0xcb, 0xb4 } });


}

#pragma     once

#include    "RSObject.h"
#include    "FECmdBuffer.h"
#include    "FERenderPass.h"
#include    "FEGPipeline.h"
#include    "FEShader.h"
#include    "FESwapchain.h"
#include    "FEGImage.h"
#include    "FEQueue.h"
#include    "FEDSetLayout.h"
#include    "FEDSetPool.h"
#include    "FEDSet.h"
#include    "FEFence.h"
#include    "FESemaphore.h"
#include    "FEGPUBuffer.h"
#include    "FETexture.h"
#include    "FELight.h"
#include    "FELightMgr.h"

namespace   FE
{
    class   FEImGuiBackend;
    DEFINE_CLASS_UUID(FEDevice,"{0C870EE5-B0CB-445B-A23A-DE0639B582DB}");
    /// <summary>
    /// 
    /// </summary>
    class   FE_API FEDevice : public FEObject
    {
    public:
        using   NotifyCreate    =   std::function<void(FEDevice&)>;
        using   NotifyDestroy   =   std::function<void(FEDevice&)>;
        using   PipelineMgr     =   FEKeyValues<String,Pipeline>;
    public:
        struct  CreateInfo
        {
            FEUuid      deviceId;
            PCSTRs      extensions;
        };
    public:
        FEDevice(FEContext& ctx)
            :FEObject(ctx)
        {
            _lightsMgr  =   new FELightMgr(_ctx);
        }
        FEDevice(const FEDevice& other)
            :FEObject(other)
        {
            _lightsMgr  =   new FELightMgr(_ctx);
        }
        virtual ~FEDevice()
        {}
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const   auto&       cInfo() const
        {
            return  _cInfo;
        }
        const   auto&       enableDeviceExtensions() const
        {
            return  _cInfo.extensions;
        }
        inline  auto        graphicCmdPool()  const
        {
            return  _graphicPool;
        }
        inline  auto        computeCmdPool()  const
        {
            return  _computePool;
        }
        inline  auto        transferCmdPool()  const
        {
            return  _transferPool;
        }
        const   auto&       pipelines() const
        {
            return  _pipelineMgr;
        }
        inline  auto&       pipelines()
        {
            return  _pipelineMgr;
        }
        const   auto&       lights() const
        {
            return  *_lightsMgr;
        }
        inline  auto&       lights()
        {
            return  *_lightsMgr;
        }
        virtual Queue       queueGraphic()          =   0;
        virtual Queue       queueCompute()          =   0;
        virtual Queue       queueTransfer()         =   0;

        virtual FEResult    create(const CreateInfo& cInfo) =   0;
        
        virtual void        waitIdle()              =   0;
        
        virtual Fence       createFence()           =   0;
        virtual Semaphore   createSemaphore()       =   0;
        virtual CMDPool     createCmdPool()         =   0;
        virtual FBOPtr      createFrameBuffer()     =   0;
        virtual Shader      createShader()          =   0;
        virtual RenderPass  createRenderPass()      =   0;
        virtual GPipeline   createGPipeline()       =   0;
        virtual Pipeline    createCPipeline()       =   0;
        virtual Swapchain   createSwapchain()   =   0;
        virtual GImage      createGImage()      =   0;
        /// <summary>
        /// 创建纹理对象(image+view+sampler)。
        /// 纹理支持当前仅 Vulkan 实现,未实现的后端返回 nullptr。
        /// </summary>
        virtual TexPtr      createTexture()
        {
            return  nullptr;
        }
        /// <summary>
        /// 图像像素回读到主机内存(离屏/无头渲染用;未实现的后端返回 false)。
        /// dst 按 format 紧密排布,size 需不小于 宽*高*每像素字节。
        /// </summary>
        virtual bool        readbackImage(GImage image,void* dst,uint64 size)
        {
            UNUSED(image,dst,size);
            return  false;
        }
        /// <summary>
        /// 创建调试 GUI(ImGui)渲染后端。
        /// 支持的后端(如 Vulkan)返回具体实现,未实现的后端返回 nullptr。
        /// 调用者负责在退出时 destroy() 并 delete。
        /// </summary>
        virtual FEImGuiBackend* createImGuiBackend()
        {
            return  nullptr;
        }
        virtual DSetLayout  createDSLayout()        =   0;
        virtual DSetPool    createDSetPool()        =   0;
        virtual DSet        createDSet()            =   0;
        virtual VBO         createVBO()             =   0;
        virtual IBO         createIBO()             =   0;
        virtual UBO         createUBO()             =   0;
        virtual ITO         createITO()             =   0;
        virtual SBO         createSBO()             =   0;

        inline  Pipeline    createPipeline(const char* typeName)
        {
            if (_stricmp(typeName,"compute") == 0)  
                return  createCPipeline().get();
            else
                return  createGPipeline().get();;

        }
        /// <summary>
        /// cpu buffer for trans src
        /// </summary>
        /// <param name=""></param>
        /// <param name="length"></param>
        /// <returns></returns>
        virtual GPUBuffer   createFromBuffer(const void*,uint64 length)   =   0;
        /// <summary>
        /// 主要作用是缓冲系统内置的对象
        /// </summary>
        /// <param name="object"></param>
        /// <returns></returns>
        virtual bool        cacheObject(Object object);
        /// <summary>
        /// 查询内置对象
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        virtual Object      queryCache(const FEUuid& id) const;
        /// <summary>
        /// 销毁
        /// </summary>
        virtual void        destroy();
        /// <summary>
        /// 是否支持 WGSL 文本 shader(WGDevice 返回 true,VKDevice 返回 false)。
        /// 用于 FEPipelineHelper 在加载 .spv 时决定是否优先查找同名 .wgls 兄弟文件。
        /// </summary>
        virtual bool        supportWGSLShaders() const
        {
            return  false;
        }
    public:
        NotifyCreate    _createNotify;
        NotifyDestroy   _destroyNotify;
    protected:
        CreateInfo      _cInfo;
        CMDPool         _graphicPool;
        CMDPool         _computePool;
        CMDPool         _transferPool;
        Objects         _cacheObject;
        /// <summary>
        /// 管理系统中的所有管线
        /// </summary>
        PipelineMgr     _pipelineMgr;
        /// <summary>
        /// 管理灯光对象
        /// </summary>
        LightMgr        _lightsMgr;
        
    };
    using   Device  =   SharedPtr<FEDevice>;
}

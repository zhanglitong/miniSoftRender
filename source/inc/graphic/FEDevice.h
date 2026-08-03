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
#include    "FELight.h"
#include    "FELightMgr.h"

namespace   FE
{
    DEFINE_CLASS_UUID(FEDevice,"{0C870EE5-B0CB-445B-A23A-DE0639B582DB}");
    /// <summary>
    /// 
    /// </summary>
    class   FEDevice : public FEObject
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
        virtual Swapchain   createSwapchain()       =   0;
        virtual GImage      createGImage()          =   0;
        virtual DSetLayout  createDSLayout()        =   0;
        virtual DSetPool    createDSetPool()        =   0;
        virtual DSet        createDSet()            =   0;
        virtual VBO         createVBO()             =   0;
        virtual IBO         createIBO()             =   0;
        virtual UBO         createUBO()             =   0;
        virtual ITO         createITO()             =   0;
        virtual SBO         createSBO()             =   0;
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
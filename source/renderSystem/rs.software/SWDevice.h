#pragma     once
#include    "graphic/FEDevice.h"
#include    "graphic/FECmdPool.h"

namespace   FE
{
    class   SWRenderSystem;

    class   SWDevice :public FEDevice
    {
    public:
        SWRenderSystem& _renderSys;
    public:
        SWDevice(FEContext& ctx,SWRenderSystem& renderSys)
            :FEDevice(ctx)
            ,_renderSys(renderSys)
        {}

        SWDevice(const SWDevice& other)
            :FEDevice(other)
            ,_renderSys(other._renderSys)
        {}

        virtual ~SWDevice();

        inline  auto&       renderSystem() const
        {
            return  _renderSys;
        }
        virtual FEResult    create(const CreateInfo& cInfo) override;

        virtual Queue       queueGraphic()      override;
        virtual Queue       queueCompute()      override;
        virtual Queue       queueTransfer()     override;

        virtual void        waitIdle()          override;
        virtual Fence       createFence()       override;
        virtual Semaphore   createSemaphore()   override;

        virtual CMDPool     createCmdPool()     override;
        virtual FBOPtr      createFrameBuffer() override;
        virtual Shader      createShader()      override;
        virtual RenderPass  createRenderPass()  override;
        virtual GPipeline   createGPipeline()   override;
        virtual Swapchain   createSwapchain()   override;
        virtual GImage      createGImage()      override;

        virtual DSetLayout  createDSLayout()    override;
        virtual DSetPool    createDSetPool()    override;
        virtual DSet        createDSet()        override;

        virtual VBO         createVBO()         override;
        virtual IBO         createIBO()         override;
        virtual UBO         createUBO()         override;
        virtual ITO         createITO()         override;
        virtual SBO         createSBO()         override;

        virtual GPUBuffer   createFromBuffer(const void*,uint64 length) override;
    protected:
        Queue   _queueGraphic   =   {};
        Queue   _queueCompute   =   {};
        Queue   _queueTransfer  =   {};
    };
}

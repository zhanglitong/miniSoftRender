#pragma     once

#include    "graphic/FEDevice.h"
#include    "graphic/FECmdPool.h"
#include    "graphic/FERenderSystem.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGRenderSystem;

    class   WGDevice :public FEDevice
    {
    public:
        WGRenderSystem&    _renderSys;
    public:
        WGDevice(FEContext& ctx,WGRenderSystem& renderSys)
            :FEDevice(ctx)
            , _renderSys(renderSys)
        {}

        WGDevice(const WGDevice& other)
            :FEDevice(other)
            , _renderSys(other._renderSys)
        {}

        virtual ~WGDevice();

        inline auto&    renderSystem() const
        {
            return _renderSys;
        }
        inline auto    device() const
        {
            return _nativeDevice;
        }
        inline auto    queue() const
        {
            return _queue;
        }

        inline  auto        physicalDevice() const
        {
            return  _gpuInfo.gpu;
        }

        virtual FEResult    create(const CreateInfo& cInfo) override;

        virtual Queue       queueGraphic() override;
        virtual Queue       queueCompute() override;
        virtual Queue       queueTransfer() override;

        virtual void        waitIdle() override;
        virtual Fence       createFence() override;
        virtual Semaphore   createSemaphore() override;

        virtual CMDPool     createCmdPool()     override;
        virtual FBOPtr      createFrameBuffer() override;
        virtual Shader      createShader()      override;
        virtual RenderPass  createRenderPass()  override;
        virtual GPipeline   createGPipeline()   override;
        virtual Pipeline    createCPipeline()   override;
        virtual Swapchain   createSwapchain()   override;
        virtual GImage      createGImage()      override;

        virtual DSetLayout  createDSLayout()    override;
        DSetLayout          createDSLayoutFromShaders(const std::vector<Shader>& shaders);
        virtual DSetPool    createDSetPool() override;
        virtual DSet        createDSet() override;

        virtual VBO    createVBO() override;
        virtual IBO    createIBO() override;
        virtual UBO    createUBO() override;
        virtual ITO    createITO() override;
        virtual SBO    createSBO() override;

        virtual GPUBuffer    createFromBuffer(const void*,uint64 length) override;

        WGPUTextureFormat    getWGPUTextureFormat(FEFormat format) const;
        bool    hasPassthroughShaders() const { return _hasPassthroughShaders; }

    protected:
        WGPUDevice    _nativeDevice    =   nullptr;
        WGPUQueue     _queue           =   nullptr;
        GPU           _gpuInfo;
        Queue         _queueGraphic    =   nullptr;
        Queue         _queueCompute    =   nullptr;
        Queue         _queueTransfer   =   nullptr;
        bool          _hasPassthroughShaders = false;
    };
}

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
        /// <summary>
        /// 当前窗口交换链的颜色格式(无交换链时为 RGBA8Unorm)。
        /// 引擎的 MSAA/场景/探针等颜色目标均跟随交换链格式,而管线在创建时
        /// 必须声明与附件一致的格式,因此这里作为管线默认颜色格式的唯一来源。
        /// </summary>
        inline  WGPUTextureFormat   surfaceFormat() const
        {
            return  _surfaceFormat;
        }
        inline  void    setSurfaceFormat(WGPUTextureFormat fmt)
        {
            _surfaceFormat  =   fmt;
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
        virtual TexPtr      createTexture()     override;

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
        /// 是否启用了 Float32Filterable(决定 float 纹理能否线性采样)
        bool    hasFloat32Filterable() const { return _hasFloat32Filterable; }
        /// <summary>
        /// 采样数(MSAA):WebGPU 仅支持 1x/4x
        /// </summary>
        virtual uint    maxSampleCount() const override
        {
            return  4;
        }
        /// WGDevice 通过 WGPUShaderSourceWGSL 直接加载 WGSL 文本 shader
        virtual bool    supportWGSLShaders() const override
        {
            return  true;
        }

    protected:
        WGPUDevice    _nativeDevice    =   nullptr;
        WGPUQueue     _queue           =   nullptr;
        GPU           _gpuInfo;
        Queue         _queueGraphic    =   nullptr;
        Queue         _queueCompute    =   nullptr;
        Queue         _queueTransfer   =   nullptr;
        bool          _hasPassthroughShaders = false;
        bool          _hasFloat32Filterable  = false;
        /// 交换链颜色格式(无窗口时保持 RGBA8Unorm,与离屏目标一致)
        WGPUTextureFormat   _surfaceFormat   =   WGPUTextureFormat_RGBA8Unorm;
    };
}

#pragma     once
#include    "graphic/FEDevice.h"
#include    "graphic/FECmdPool.h"
#include    "graphic/FETexture.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKRenderSystem;

    using   DeviceProperties        =   VkPhysicalDeviceProperties;
    using   DeviceFeatures          =   VkPhysicalDeviceFeatures;
    using   MemoryProperties        =   VkPhysicalDeviceMemoryProperties;
    using   QueueFamilyProperties   =   std::vector<VkQueueFamilyProperties>;
    using   VkFormats               =   std::vector<VkFormat>;

    class   VKDevice :public FEDevice
    {
    public:
        VKRenderSystem& _renderSys;
    public:
        VKDevice(FEContext& ctx,VKRenderSystem& renderSys)
            :FEDevice(ctx)
            ,_renderSys(renderSys)
        {}

        VKDevice(const VKDevice& other)
            :FEDevice(other)
            ,_renderSys(other._renderSys)
        {}

        virtual ~VKDevice();
        
        inline  auto&       renderSystem() const
        {
            return  _renderSys;
        }
        inline  auto        physicalDevice() const
        {
            return  _physicalDevice;
        }
        inline  auto        logicalDevice() const
        {
            return  _logicalDevice;
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
        virtual Pipeline    createCPipeline()   override;
        virtual Swapchain   createSwapchain()   override;
        virtual GImage      createGImage()      override;
        virtual TexPtr      createTexture()     override;
        virtual bool        readbackImage(GImage image,void* dst,uint64 size) override;
        virtual FEImGuiBackend* createImGuiBackend() override;
        /// <summary>
        /// 物理设备支持的最大采样数(MSAA),颜色/深度取样交集
        /// </summary>
        virtual uint        maxSampleCount() const override;

        virtual DSetLayout  createDSLayout()    override;
        DSetLayout          createDSLayoutFromShaders(const std::vector<Shader>& shaders);
        virtual DSetPool    createDSetPool()    override;
        virtual DSet        createDSet()        override;

        virtual VBO         createVBO()         override;
        virtual IBO         createIBO()         override;
        virtual UBO         createUBO()         override;
        virtual ITO         createITO()         override;
        virtual SBO         createSBO()         override;

        virtual GPUBuffer   createFromBuffer(const void*,uint64 length) override;
    public:
        uint32      getMemoryType(uint32_t typeBits
                                , VkMemoryPropertyFlags properties
                                , VkBool32 *memTypeFound = nullptr) const;
        uint32      getQueueFamilyIndex(VkQueueFlags queueFlags) const;

        uint32      getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);

        VkResult    createLogicalDevice(DeviceFeatures enabledFeatures, const PCSTRs& enabledExtensions, void *pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
		
		bool        extensionSupported(const String& extension)
		{
			return (std::find(_supportedExtensions.begin(), _supportedExtensions.end(), extension) != _supportedExtensions.end());
		}

		VkFormat	getSupportedDepthFormat(bool checkSamplingSupport);
        /// <summary>
        /// 用临时 cmd buffer 把 image 全链从 UNDEFINED 切到 SHADER_READ_ONLY,
        /// 用于 renderTarget(multi-mip)刚创建/resize 后的初始状态,
        /// 避免第 1 帧 pass1 分支未进时 shader 采样 UNDEFINED image 触发验证层报错
        /// </summary>
        void    initializeImageToShaderReadable(GImage img);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        bool        supportExtension(const char* name) const
        {
            auto    itr =   std::find(_supportedExtensions.begin(),_supportedExtensions.end(),name);
            if (itr == _supportedExtensions.end())
                return  false;
            else
                return  true;
        }
    protected:
		VkPhysicalDevice	    _physicalDevice         =   VK_NULL_HANDLE;
		VkDevice			    _logicalDevice          =   VK_NULL_HANDLE;
        VkCommandPool		    _commandPool            =   VK_NULL_HANDLE;
        DeviceProperties        _properties             =   {};
        DeviceFeatures          _features               =   {};
        DeviceFeatures          _enabledFeatures        =   {};
        MemoryProperties        _memoryProperties       =   {};
        QueueFamilyProperties   _queueFamilyProperties  =   {};
		Strings				    _supportedExtensions    =   {};

        Queue                   _queueGraphic           =   {};       
        Queue                   _queueCompute           =   {};
        Queue                   _queueTransfer          =   {};
		struct  QueueFamilyIndex
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
		};
        QueueFamilyIndex    _queueFamilyIndices    =   {};
    };
}

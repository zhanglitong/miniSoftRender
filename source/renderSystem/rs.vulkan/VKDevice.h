#pragma     once
#include    "graphic/FEDevice.h"
#include    "graphic/FECmdPool.h"
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
        virtual Swapchain   createSwapchain()   override;
        virtual GImage      createGImage()      override;

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
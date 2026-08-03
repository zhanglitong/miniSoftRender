#pragma     once
#include    "graphic/FERenderSystem.h"
#include    "VKDevice.h"
#include    "VKInstanceLayers.h"
#include    "VKInstanceExtensions.h"

namespace   FE
{
    inline  static  PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessenger     =   nullptr;
    inline  static  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger    =   nullptr;

    DEFINE_CLASS_UUID(VKRenderSystem,"{6BA6DF09-E1D8-449B-893E-140952BD6BEE}");
    class   VKRenderSystem :public FERenderSystem
    {
        IMPLEMENT_CLASS_REFLECT(VKRenderSystem)
    public:
        using   PhysicalDevices     =   std::vector<VkPhysicalDevice>;
        using   VkDebuger           =   VkDebugUtilsMessengerEXT;
    public:
        VKRenderSystem(FEContext& ctx)
            :FERenderSystem(ctx)
        {}
        VKRenderSystem(const VKRenderSystem& other)
            :FERenderSystem(other)
        {}
        virtual ~VKRenderSystem();

        inline  auto        instance() const
        {
            return  _native;
        }

        /// <summary>
        /// 
        /// </summary>
        virtual bool    isValid() const
        {
            return  _native != nullptr;
        }
        /// <summary>
        /// 
        /// </summary>
        virtual void    initialize() 
        {}
        /// <summary>
        /// 
        /// </summary>
        virtual void    destroy();
        /// <summary>
        /// 
        /// </summary>
        virtual Handle  native()  const 
        {
            return  Handle(_native);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        virtual const GPUs& gpuList()   const   override
        {
            return  _gpus;
        }
        /// <summary>
        /// �����豸
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        virtual FEResult    create()            override
        {
            LOG_DBG("VKRenderSystem.create");
            auto    result  =   createInstanceImpl();
            _gpus           =   gpuListImpl();
            return  result;
        }
        virtual Device      createDevice()      override
        {
            LOG_DBG("VKRenderSystem.createDevice");
            return  new VKDevice(_ctx,*this);
        }
        
    protected:
        FEResult    createInstanceImpl();
        GPUs        gpuListImpl() const;
        void        debug(VkDebugUtilsMessageSeverityFlagBitsEXT flg,const char* msg);
    public:
        static  VKAPI_ATTR VkBool32 debugCallback(    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
                                                    , VkDebugUtilsMessageTypeFlagsEXT messageType
                                                    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
                                                    , void* pUserData);
    protected:
        VkInstance  _native     =   nullptr;
        VkDebuger   _debuger    =   nullptr;
        GPUs        _gpus;
        bool        _enableValidationLayers =   true;
    };
}
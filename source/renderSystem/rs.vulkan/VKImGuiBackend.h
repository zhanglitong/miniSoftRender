#pragma     once

#include    "graphic/FEImGuiBackend.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKDevice;
    /// <summary>
    /// 调试 GUI(ImGui)的 Vulkan 渲染后端。
    /// 引擎使用 VK_KHR_dynamic_rendering(无 VkRenderPass),因此无法使用官方
    /// imgui_impl_vulkan,这里自写一个最小渲染器:图形管线用
    /// VkPipelineRenderingCreateInfo 创建(颜色格式与交换链一致,
    /// 深度/模板附件格式取引擎实际附件 D32_SFLOAT_S8_UINT),
    /// 顶点格式与 ImGui 一致,每帧把顶点/索引数据拷进常驻映射的宿主可见缓冲,
    /// 再用 vkCmdDrawIndexed 录制;绘制结束复位 scissor,避免污染同帧其它工厂。
    /// </summary>
    class   VKImGuiBackend :public FEImGuiBackend
    {
    public:
        /// 顶点/索引缓冲容量(足够调试面板使用)
        static  constexpr   uint32  kMaxVertices    =   1u << 17;   ///< 131072
        static  constexpr   uint32  kMaxIndices     =   1u << 18;   ///< 262144
    public:
        VKImGuiBackend(FEContext& ctx,VKDevice& device);
        virtual ~VKImGuiBackend();

        virtual bool    init(FEFormat colorFormat)  override;
        virtual bool    uploadFont(const FEImGuiFontData& font) override;
        virtual void    render(const FEImGuiDrawData& drawData,FECmdBuffer* cmd) override;
        virtual void    destroy()   override;
    protected:
        bool    createPipeline();
        bool    createDescriptorSet();
        bool    createFontImage(uint32 width,uint32 height,const void* pixels);
        bool    createMappedBuffer(VkDeviceSize size,VkBufferUsageFlags usage,
                                   VkBuffer& buffer,VkDeviceMemory& memory,void*& mapped);
        /// 一次性命令缓冲:录制 uploadFont 所需的拷贝
        void    uploadImage(const void* pixels,VkDeviceSize size);
    protected:
        FEContext&              _ctx;
        VKDevice&               _device;
        VkFormat                _colorFormat    =   VK_FORMAT_UNDEFINED;

        VkShaderModule          _vertModule     =   VK_NULL_HANDLE;
        VkShaderModule          _fragModule     =   VK_NULL_HANDLE;
        VkDescriptorSetLayout   _dsLayout       =   VK_NULL_HANDLE;
        VkPipelineLayout        _plLayout       =   VK_NULL_HANDLE;
        VkDescriptorPool        _dsPool         =   VK_NULL_HANDLE;
        VkDescriptorSet         _dsSet          =   VK_NULL_HANDLE;
        VkPipeline              _pipeline       =   VK_NULL_HANDLE;
        VkSampler               _sampler        =   VK_NULL_HANDLE;

        VkImage                 _fontImage      =   VK_NULL_HANDLE;
        VkDeviceMemory          _fontMemory     =   VK_NULL_HANDLE;
        VkImageView             _fontView       =   VK_NULL_HANDLE;
        VkExtent3D              _fontExtent     =   { 0,0,1 };

        VkBuffer                _vtxBuffer      =   VK_NULL_HANDLE;
        VkDeviceMemory          _vtxMemory      =   VK_NULL_HANDLE;
        void*                   _vtxMapped      =   nullptr;
        VkBuffer                _idxBuffer      =   VK_NULL_HANDLE;
        VkDeviceMemory          _idxMemory      =   VK_NULL_HANDLE;
        void*                   _idxMapped      =   nullptr;
    };
}

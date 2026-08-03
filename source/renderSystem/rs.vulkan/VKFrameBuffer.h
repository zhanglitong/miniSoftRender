#pragma     once
#include    "graphic/FEFrameBuffer.h"
#include    "FEVulkan.h"

namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   VKFrameBuffer :public TRSObject<VkFramebuffer,FEFrameBuffer>
    {
    public:
        VKFrameBuffer(FEContext& ctx)
            :TRSObject<VkFramebuffer,FEFrameBuffer>(ctx)
        {}

        VKFrameBuffer(const VKFrameBuffer& other)
            :TRSObject<VkFramebuffer,FEFrameBuffer>(other)
        {}
        virtual ~VKFrameBuffer();

        virtual bool    create(const CreateInfo& info)  override;
    };
}
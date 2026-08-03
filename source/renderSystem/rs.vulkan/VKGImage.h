#pragma     once

#include    "graphic/FEGImage.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKGImage :public TRSObject<VkImage,FEGImage>
    {
    protected:
        bool            _ref    =   false;
        VkDeviceMemory  _memory =   nullptr;
    public:
        VKGImage(FEContext& ctx,VkImage native = nullptr,bool bRef = false)
            :TRSObject<VkImage,FEGImage>(ctx,native)
        {
            _ref    =   bRef;
        }
        VKGImage(const VKGImage& other)
            :TRSObject<VkImage,FEGImage>(other)
        {}
        virtual ~VKGImage();

        virtual bool        create(const CreateInfo& info)  override;
        virtual GImgView    createView() override;
    };
}
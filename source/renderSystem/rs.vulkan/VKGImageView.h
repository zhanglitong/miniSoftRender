#pragma     once

#include    "graphic/FEGImageView.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKGImageView :public TRSObject<VkImageView,FEGImageView>
    {
    protected:
        bool    _ref    =   false;
    public:
        VKGImageView(FEContext& ctx,VkImageView native = nullptr,bool bRef = false)
            :TRSObject<VkImageView,FEGImageView>(ctx,native)
        {
            _ref    =   bRef;
        }
        VKGImageView(const VKGImageView& other)
            :TRSObject<VkImageView,FEGImageView>(other)
        {}
        virtual ~VKGImageView();

        inline  void    setImage(FEGImage* image)
        {
            _cInfo._image   =   image;
        }

        virtual bool    create(const CreateInfo& info)  override;
    };
}
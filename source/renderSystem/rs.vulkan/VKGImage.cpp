
#include    "VKGImage.h"
#include    "VKDevice.h"
#include    "VKGImageView.h"

namespace   FE
{
    VKGImage::~VKGImage()
    {
        if(!_ref && _native != nullptr)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyImage(device,  _native, nullptr);
            vkFreeMemory(device,    _memory,nullptr);
        }
    }

    bool    VKGImage::create(const FEGImage::CreateInfo& info)
    {
        VkImageCreateInfo imageCI  =  {};

        _cInfo              =   info;

        auto&   vkDevice        =   (VKDevice&)_ctx.device();
        auto    device          =   vkDevice.logicalDevice();
    
        imageCI.sType           =   VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType       =   system2Native(info._type);
        imageCI.format          =   system2Native(info._format);
        imageCI.extent          =   { info._width,info._height, info._depth };
        imageCI.mipLevels       =   info._mips;
        imageCI.arrayLayers     =   info._layer;
        imageCI.samples         =   VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling          =   VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage           =   system2Native(info._usage);
        imageCI.initialLayout   =   system2Native(info._layout); 
    
        VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &_native));
        VkMemoryRequirements    memReqs{};
        vkGetImageMemoryRequirements(device, _native, &memReqs);

        VkMemoryAllocateInfo    memAllloc  =   {};
        {
          memAllloc.sType           =   VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
          memAllloc.allocationSize  =   memReqs.size;
          memAllloc.memoryTypeIndex =   vkDevice.getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &_memory));
        VK_CHECK_RESULT(vkBindImageMemory(device,_native, _memory, 0));

        return  _native != nullptr;
    }


    void    VKGImage::destroy()
    {
        if(!_ref && _native != nullptr)
        {
            auto&   vkDevice    =   (VKDevice&)_ctx.device();
            auto    device      =   vkDevice.logicalDevice();
            vkDestroyImage(device,  _native,    nullptr);
            vkFreeMemory(device,    _memory,    nullptr);
            _native =   nullptr;
            _memory =   nullptr;
        }
    }

    GImgView    VKGImage::createView()
    {
        GImgView                    view    =   new VKGImageView(_ctx);
        VKGImageView::CreateInfo    info    =   {};
        info._image         =   this;
        if (view->create(info))
            return  view;
        else
            return  nullptr;
    }
}

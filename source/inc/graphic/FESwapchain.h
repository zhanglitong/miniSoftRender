#pragma     once

#include    "RSObject.h"
#include    "FEGImageView.h"
#include    "FEFence.h"
#include    "FESemaphore.h"
#include    "FEQueue.h"

namespace   FE
{
    class   FEDevice;
    using   Device  =   SharedPtr<FEDevice>;
    
    class   FESwapchain :public RSObject
    {
    public:
        struct  CreateInfo
        {
            void*   _appInst            =   nullptr;
            void*   _window             =   nullptr;
            uint    _width              =   0;
            uint    _height             =   0;
            uint    _queueFamilyIndex   =   0;
        };
        struct  PresentInfo
        {
            Semaphore   _sem;
            uint        _imageIndex;
            Queue       _queue;
        };
        CreateInfo  _cInfo;
    public:
        FESwapchain(FEContext& ctx)
            :RSObject(ctx)
        {}
        FESwapchain(const FESwapchain& other)
            :RSObject(other)
        {}
        const auto&     cInfo() const
        {
            return  _cInfo;
        }
    public:
        virtual bool        acquireNextImage(uint64 timeout, Semaphore sem, Fence fence,uint& imageIndex)   =   0;
        virtual GImgViews   imageViews() const  =   0;
        virtual bool        create(const CreateInfo& info)  =   0;

        virtual bool        queuePresent(const PresentInfo& pInfo)    =   0;
        virtual FEFormat    colorFormat() const     =   0;

    };


    using   Swapchain =   SharedPtr<FESwapchain>;
}
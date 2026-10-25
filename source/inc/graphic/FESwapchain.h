#pragma     once

#include    "RSObject.h"
#include    "FEGImage.h"
#include    "FEGImageView.h"
#include    "FEFence.h"
#include    "FESemaphore.h"
#include    "FEQueue.h"
#include    "FEFrame.h"

namespace   FE
{
    class   FE_API  FESwapchain :public RSObject
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
            Queue       _queue;
            Frame       _frame;
        };
        CreateInfo  _cInfo;
    public:
        FESwapchain(FEContext& ctx)
            :RSObject(ctx)
        {}
        FESwapchain(const FESwapchain& other)
            :RSObject(other)
        {}
        virtual~FESwapchain()
        {
        }
        const auto&     cInfo() const
        {
            return  _cInfo;
        }
    public:
        virtual Frame       acquireNextFrame(uint64 timeout)    =   0;
        virtual bool        create(const CreateInfo& info)      =   0;
        virtual bool        queuePresent(const PresentInfo& pInfo)    =   0;
        virtual FEFormat    colorFormat() const     =   0;
        /// <summary>
        /// 离屏渲染目标:无头(离屏)交换链返回色附件图像,窗口交换链返回 nullptr
        /// </summary>
        virtual GImage      colorImage() const      {   return  nullptr;    }
    };


    using   Swapchain =   SharedPtr<FESwapchain>;
}

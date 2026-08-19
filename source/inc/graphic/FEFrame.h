#pragma     once

#include    "RSObject.h"
#include    "FEGImageView.h"
#include    "FEFence.h"
#include    "FESemaphore.h"
#include    "FECmdBuffer.h"

namespace   FE
{
    class   FEDevice;
    using   Device  =   SharedPtr<FEDevice>;
    
    class   FEFrame :public FEObject
    {
    public:
        friend  class   FECmdBuffer;
        friend  class   FEQueue;
        friend  class   VKQueue;
        friend  class   VKSwapchain;
        friend  class   FEScene;
        friend  class   WGQueue;
        friend  class   WGSwapchain;
    public:
        FEFrame(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEFrame(const FEFrame& other)
            :FEObject(other)
        {}

        inline  void    reset()
        {
            if (_fenceWait)
            {
                _fenceWait->wait(UINT64_MAX);
                _fenceWait->reset();
            }
        }
    public:
        CMDPtr      _cmd;
    protected:
        Fence       _fenceWait;
        Semaphore   _semRenderComplete;
        Semaphore   _semPresentComplete;
        GImgView    _imageViewer;
        uint32_t    _imageIdx   =   MaxUint32;
        
    };


    using   Frame   =   SharedPtr<FEFrame>;
    using   Frames  =   std::vector<Frame>;
}

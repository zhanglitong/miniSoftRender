#pragma     once
#include    "graphic/FEFrameBuffer.h"

namespace   FE
{
    class   SWFrameBuffer :public TRSObject<void*,FEFrameBuffer>
    {
    public:
        SWFrameBuffer(FEContext& ctx)
            :TRSObject<void*,FEFrameBuffer>(ctx)
        {}

        SWFrameBuffer(const SWFrameBuffer& other)
            :TRSObject<void*,FEFrameBuffer>(other)
        {}
        virtual ~SWFrameBuffer();

        virtual bool    create(const CreateInfo& info)  override;
    };
}

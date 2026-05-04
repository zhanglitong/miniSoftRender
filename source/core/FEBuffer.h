#pragma     once

#include    "../inc/FEDefine.h"
#include    "../inc/FEObject.h"
#include    "../inc/FEMath.hpp"
#include    "FEStruct.h"

namespace   FE
{
    class   FE_API  FEBuffer :public FEObject
    {
    protected:
        bytes   _buffer;
    public:
        virtual ~FEBuffer() =   default;

        virtual CallResult  create(const FEBufferInfor& infor) 
        {
            _buffer.resize(infor._size);
            return  CallResult::CR_SUCCESS;
        }
    };

    using   BufferPtr   =   SharedPtr<FEBuffer>;
}
#pragma     once

#include    "RSObject.h"

namespace   FE
{
    class   FECmdBuffer;
    using   CMDPtr  =   SharedPtr<FECmdBuffer>;
    class   FECmdPool :public RSObject
    {
    public:
        struct  CreateInfo
        {
            /// <summary>
            ///  swapChain._queueFamilyIndex
            /// </summary>
            uint    _queueFamilyIndex   =   0;
        };
    public:
        FECmdPool(FEContext& ctx)
            :RSObject(ctx)
        {}
        FECmdPool(const FECmdPool& other)
            :RSObject(other)
        {}
        virtual bool        create(const CreateInfo& cInf)      =   0;
        virtual CMDPtr      createCmd()    =   0;
        
    };
    using   CMDPool =   SharedPtr<FECmdPool>;
}

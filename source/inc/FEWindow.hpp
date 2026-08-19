#pragma     once
#include    "FEObject.h"
#include    "FEPlatform.h"
#include    "FEInput.hpp"

namespace   FE
{
    class   FEContext;

    class   FEWindow
        :public FEObject
        ,public FEInput
    {
    public:
        FEWindow(FEContext& ctx)
            :FEObject(ctx)
        {
        }   
        FEWindow(const FEWindow& other)
            :FEObject(other)
        {
        } 
        virtual ~FEWindow()
        {
        }
        virtual RectU32     rect() const = 0;

        inline uint32_t     width() const
        {
            return  rect().width();
        }
        inline uint32_t    height() const
        {
            return  rect().height();
        }
    };
    using   Window      =   SharedPtr<FEWindow>;
}

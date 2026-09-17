#pragma     once

#include    "FEMessage.hpp"
namespace   FE
{
    class   FE_API FEInput
    {
    public:
        virtual void    onMessage(const FEMessage& msg) =   0;
    };
}

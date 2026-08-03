#pragma     once

#include    "FEMessage.hpp"
namespace   FE
{
    class   FEInput
    {
    public:
        virtual void    onMessage(const FEMessage& msg) =   0;
    };
}
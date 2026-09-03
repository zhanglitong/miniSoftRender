#pragma     once

#include    "FEMessage.hpp"
#include    "FEInput.hpp"
#include    "FEComponent.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEInputCom, "{4AAA639C-889E-4D10-B287-FB9C6F23BD1E}");

    class   FEInputCom 
        :public FEComponent
        ,public FEInput
    {
        IMPLEMENT_CLASS_REFLECT(FEInputCom) 
    public:
        FEInputCom(FEContext& ctx)
            :FEComponent(ctx)
        {}

        FEInputCom(const FEInputCom& other)
            :FEComponent(other)
        {}
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        virtual void    onMessage(const FEMessage& ) override
        {}
    };

    using   InputCom    =   SharedPtr<FEInputCom>;
}

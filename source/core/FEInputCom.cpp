
#include    "../inc/FEInputCom.hpp"
#include    "../inc/FEContext.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/FEInputSystem.hpp"

namespace   FE
{
    using   Input   =   SharedPtr<FEInputCom>;

    FEInputCom::FEInputCom(FEContext& ctx)
        :FEComponent(ctx)
    {
        _priority.setPriority(EP_Second);
    }
    FEInputCom::FEInputCom(const FEInputCom& other)
        :FEComponent(other)
    {
        _priority.setPriority(other._priority.priority());
    }
    Input   FEInputCom::captureObject() const
    {   
        auto    sys =   _ctx.scene()->inputSystem();
        if (sys)
            return  sys->captureObject();
        else
            return  nullptr;
    }
    bool    FEInputCom::setCapture() 
    {
        auto    sys =   _ctx.scene()->inputSystem();
        if (sys)
            return  sys->setCapture(this);
        else
            return  false;
    }
    bool    FEInputCom::releaseCapture()
    {
        auto    sys =   _ctx.scene()->inputSystem();
        if (sys)
            return  sys->releaseCapture(this);
        else
            return  false;
    }
    void    FEInputCom::onMessage(const FEMessage& )
    {}
}

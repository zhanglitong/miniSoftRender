
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../inc/graphic/FESceneBrowse.h"

namespace   FE
{
    FESceneBrowse::FESceneBrowse(FEContext& ctx)
        :FEInputCom(ctx)
    {
        _priority.setPriority(EP_Last);
        _priority.setOrder(MaxInt16);
    }

    FESceneBrowse::FESceneBrowse(const FESceneBrowse& other)
        :FEInputCom(other)
    {
        _priority   =   other._priority;
    }
    FESceneBrowse::~FESceneBrowse()
    {}

    void    FESceneBrowse::onMessage(const FEMessage& msg)
    {
        /// 处理输入消息的逻辑
        /// 这里可以根据具体的消息类型进行处理，例如鼠标点击、键盘输入等
        /// 可以调用相应的回调函数或者修改相机状态等
        switch (msg.msgId())
        {
        case MSG_LBUTTON_DOWN:
            onLButtonDown(static_cast<const MsgLButtonDown&>(msg));
            break;
        case MSG_RBUTTON_DOWN:
            onRButtonDown(static_cast<const MsgRButtonDown&>(msg));
            break;
        case MSG_MOUSE_MOVE:
            onMouseMove(static_cast<const MsgMouseMove&>(msg));
            break;
        case MSG_MOUSE_WHEEL:
            onMouseWheel(static_cast<const MsgMouseWheel&>(msg));
            break;
        case MSG_KEYDOWN:
            onKeyDown(static_cast<const MsgKeyDown&>(msg));
            break;
        case MSG_KEYUP:
            onKeyUp(static_cast<const MsgKeyUp&>(msg));
            break;
        default:
            break;
        }
    }
    
    void    FESceneBrowse::onLButtonDown(const MsgLButtonDown& msg)
    {
        auto    point   =   _ctx.anchor().point();
        auto    camera  =   _ctx.scene()->camera();
        if (camera == nullptr)
            return;
        /// 这里需要获取到相对于当前viewer的坐标
        auto    viewPt  =   msg._info._mouse;
        if (msg.viewer())
        {
            viewPt  =   msg.viewer()->clientToViewer(msg._info._mouse);
        }
        auto    ray     =   camera->createRayFromScreen(viewPt.x,viewPt.y);
        auto&   nodeTree=   _ctx.scene()->nodeTree();

        Pickups results;
        for (auto& node: nodeTree.topLevelNodes())
        {
            node->intersect(ray,results);
        }
        std::sort(results.begin(),results.end(),[](const FEPickup& left,const FEPickup& right)
        {
            return  left.time < right.time;
        });
        if (!results.empty())
        {
            _ctx.anchor().setPickup(results.front());
            _ctx.anchor().fireNotify();
        }  
    }

    void    FESceneBrowse::onRButtonDown(const MsgRButtonDown& msg)
    {
        UNUSED(msg);
    }
    void    FESceneBrowse::onMouseMove(const MsgMouseMove& msg)
    {
        auto    camera  =   _ctx.scene()->camera();
        if (camera == nullptr)
            return;

        if (msg._info.lButtonPressed())
        {
            int2    offset  =   msg._info._prev - msg._info._mouse;
            auto    point   =   _ctx.anchor().point();
            camera->rotateViewZByCenter(offset.x * 0.2, point);
            camera->rotateViewXByCenter(offset.y * 0.2, point);
        }
        if (msg._info.rButtonPressed())
        {
            int2    offset      =    msg._info._prev - msg._info._mouse;
            auto    point       =   _ctx.anchor().point();
            real3   right       =   camera->getRight();
            real3   up          =   camera->getUp();
            real3   eye         =   camera->getEye();
            real2   deltaD      =   camera->calcWowrldPScreen(point) * real2(offset);
            real3   eyeDelta    =   ((right * (double)deltaD.x) + (up * -(double)deltaD.y));
                    eye         +=  eyeDelta;

            camera->setEye(eye);
            camera->setTarget(camera->getTarget() + eyeDelta);
            camera->update();
        }
    }
    void    FESceneBrowse::onMouseWheel(const MsgMouseWheel& msg)
    {
        auto    point   =   _ctx.anchor().point();
        auto    camera  =   _ctx.scene()->camera();
        if (camera)
        {
            camera->scaleCameraByPos(point,msg._info._zDelta > 0 ? 1.2 : 0.8333);
        }
    }

    void    FESceneBrowse::onKeyDown(const MsgKeyDown& )
    {
    }
    void    FESceneBrowse::onKeyUp(const MsgKeyUp&)
    {}
}

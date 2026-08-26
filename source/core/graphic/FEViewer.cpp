
#include    "../inc/graphic/FEViewer.h"
#include    "../inc/graphic/FEDevice.h"
#include    "../inc/graphic/FEFactoryMgr.hpp"
#include    "../inc/graphic/FEScene.h"

namespace   FE
{
    bool    FEViewer::setup(const uint2& off,uint w, uint h, ViewerUsages usages)
    {
        if (w == MaxUint32)     w   =  _ctx .windowsWidth();
        if (h == MaxUint32)     h   =  _ctx .windowsHeight();

        if (_usages.data() == 0)
            _usages =   usages;

        _rect   =   RectU32(off.x,off.y,off.x + w,off.y + h);

        if (_fbo == nullptr)
        {
            _fbo = _ctx.device().createFrameBuffer();
            FEFrameBuffer::CreateInfo   info    =   {};
            info._width     =   w;
            info._height    =   h;
            auto    result  =   _fbo->create(info); 
            assert(result);
            if (!result)
                return  false;
        }
        if (_camera == nullptr)
        {
            _camera     =   new FECamera(_ctx);
            _camera->setViewSize(real2(w,h));
        }
        return  true;
    }

    void    FEViewer::destroy()
    {
        if (_fbo)
        {
            _fbo->destroy();
            _fbo = nullptr;
        }
        _camera =   nullptr;
        _usages =   0;
    }
    void    FEViewer::onMessage(const FEMessage& msg)
    {
        /// 处理输入消息的逻辑
        /// 这里可以根据具体的消息类型进行处理，例如鼠标点击、键盘输入等
        /// 可以调用相应的回调函数或者修改相机状态等
        switch (msg.msgId())
        {
        case MSG_UPDATE:
            {
                MsgUpdate   updateMsg   =   static_cast<const MsgUpdate&>(msg);
                updateMsg._info._usages =   _usages.data();
                onUpdate(updateMsg);
            }
            break;
        case MSG_RENDER:    
            {
                MsgRender   renderMsg   =   static_cast<const MsgRender&>(msg);
                renderMsg._info._usages =   _usages.data();
                onRender(renderMsg);
            }
            break;
        case MSG_RESIZE:
            onResize(static_cast<const MsgResize&>(msg));
            break; 
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
    void    FEViewer::onUpdate(const MsgUpdate& msg)
    {
        aabb3dr box;
        auto&   factorys    =   _ctx.scene()->factoryMgr().objects();

        for (auto& var : factorys)
        {
            if (!(msg._info._usages.data() & var->usages().data()))
                continue;
            box.merge(var->aabb());
        }
        _aabb   =   box;
    }
    void    FEViewer::onRender(const MsgRender& msg)
    {
        uint    width   =   _fbo ? _fbo->cInfo()._width : _ctx.windowsWidth();  
        uint    height  =   _fbo ? _fbo->cInfo()._height: _ctx.windowsHeight();  
        auto    cmd     =   msg._info._cmdBuffer;
        FECmdBuffer::Viewport   viewPort    =   
        {
            0.0f,0.0f,(float)width,(float)height,0.0f,1.0f
        };
        RectU32     rect(0,0,width,height);

        cmd->setViewport(0,  1,  &viewPort);
        cmd->setScissor(0,   1,  &rect);

        auto        factorys    =   _ctx.scene()->factoryMgr().objects();
        std::sort(factorys.begin(),factorys.end(),[](const FactoryRender& left,const FactoryRender& right)
        {
            auto    prioLeft    =   left->priority(FEFactory::PT_Render);
            auto    prioRight   =   right->priority(FEFactory::PT_Render);
            if(prioLeft.priority() == prioRight.priority())
                return  prioLeft.order() < prioRight.order();
            else
                return  prioLeft.priority() <  prioRight.priority();
        });
        static  constexpr ViewerUsage usageList[] =   
        {
            USAGE_Background ,
            USAGE_Shadow  ,
            USAGE_Water   ,
            USAGE_Bloom   ,
            USAGE_Defferd ,
            USAGE_Scene   ,
            USAGE_Decal   ,
            USAGE_GUI     ,
            USAGE_Overlay ,
        };
        /// 保证工厂的渲染顺序，按照usage的顺序进行渲染
        for (auto usage : usageList)
        {
            if (!_usages.hasFlag(usage))
                continue;
            for (auto var : factorys)
            {
                if (!var->usages().hasFlag(usage))
                    continue;
                var->render(cmd);
            }
        }
    }
    void    FEViewer::onResize(const MsgResize& msg)
    {
        UNUSED(msg);
        if(!flagsAs<ViewerUsages>().hasFlag(FLAG_RESIZABLE))
            return;
        if (_fbo)
            _fbo->resize(uint3(msg._info._size.x,msg._info._size.y,1));
        if(_camera)
            _camera->setViewSize(real2(msg._info._size.x,msg._info._size.y));
    }

    void    FEViewer::onLButtonDown(const MsgLButtonDown& msg)
    {
        auto    point   =   _ctx.anchor().point();
        auto    camera  =   _camera ? _camera : _ctx.scene()->camera();
        if (camera == nullptr)
            return;
        /// 这里需要获取到相对于当前viewer的坐标
        auto    viewPt  =   clientToViewer(msg._info._mouse);
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

    void    FEViewer::onRButtonDown(const MsgRButtonDown& msg)
    {
        UNUSED(msg);
    }
    void    FEViewer::onMouseMove(const MsgMouseMove& msg)
    {
        auto    camera  =   _camera ? _camera : _ctx.scene()->camera();
        if (camera == nullptr)
            return;

        if (msg._info.lButtonPressed())
        {
            int2    offset  =   msg._info._old - msg._info._mouse;
            auto    point   =   _ctx.anchor().point();
            _camera->rotateViewZByCenter(offset.x * 0.2, point);
            _camera->rotateViewXByCenter(offset.y * 0.2, point);
        }
        if (msg._info.rButtonPressed())
        {
            int2    offset      =    msg._info._old - msg._info._mouse;
            auto    point       =   _ctx.anchor().point();
            real3   right       =   _camera->getRight();
            real3   up          =   _camera->getUp();
            real3   eye         =   _camera->getEye();
            real2   deltaD      =   _camera->calcWowrldPScreen(point) * real2(offset);
            real3   eyeDelta    =   ((right * (double)deltaD.x) + (up * (double)deltaD.y));
                    eye         +=  eyeDelta;

            _camera->setEye(eye);
            _camera->setTarget(_camera->getTarget() + eyeDelta);
            _camera->update();
        }
    }
    void    FEViewer::onMouseWheel(const MsgMouseWheel& msg)
    {
        auto    point   =   _ctx.anchor().point();
        auto    camera  =   _camera ? _camera : _ctx.scene()->camera();
        if (camera)
        {
            camera->scaleCameraByPos(point,msg._info._zDelta > 0 ? 1.2 : 0.8333);
        }
    }

    void    FEViewer::onKeyDown(const MsgKeyDown& msg)
    {
        #define VK_LEFT           0x25
        #define VK_UP             0x26
        #define VK_RIGHT          0x27
        #define VK_DOWN           0x28

        Node    _pickupNode =   nullptr;

        if (_pickupNode == nullptr)
            return;
        switch(msg._info._key)
        {
        case VK_LEFT:
            if (_pickupNode)
            {
                _pickupNode->setLocalTranslation(_pickupNode->localTranslation() + real3(-1,0,0));
                _pickupNode->update();
            }
            break;
        case VK_RIGHT:
            if (_pickupNode)
            {
                _pickupNode->setLocalTranslation(_pickupNode->localTranslation() + real3(+1,0,0));
                _pickupNode->update();
            }
            break;
        case VK_UP:
            if (_pickupNode)
            {
                _pickupNode->setLocalTranslation(_pickupNode->localTranslation() + real3(0,0,1));
                _pickupNode->update();
            }
            break;
        case VK_DOWN:
            if (_pickupNode)
            {
                _pickupNode->setLocalRotation(_pickupNode->localTranslation() + real3(0,0,-1));
                _pickupNode->update();
            }
            break;
        }
    }
    void    FEViewer::onKeyUp(const MsgKeyUp&)
    {}
}

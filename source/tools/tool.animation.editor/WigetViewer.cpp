#include    "WigetViewer.h"
#include    "FEAppHelper.hpp"
#include    "FEFileFormatHelper.hpp"

namespace   FE
{

    WigetViewer::WigetViewer(QWidget* parent)
        :QWidget(parent)
    {
        setAcceptDrops(true);
        setAttribute(Qt::WA_AcceptTouchEvents);

        FEApp::CreateInfo   info    =   {};
        info._window    =   (void*)winId();
        info._width     =   rect().width();
        info._height    =   rect().height();
        info._notify    =   std::bind(&WigetViewer::messageNotify,this,std::placeholders::_1);
        _app    =   FE::FEAppHelper::create(_ctx,info);
        if (_app == nullptr)
            return;
        _scene      =   new FEScene(_ctx);
        _scene->setup(_app);
        _scene->test();
        Node    rootNode = new FENode(_ctx);
        rootNode->setName("root");
        for (size_t i = 0; i < 10; i++)
        {
            Node    child = new FENode(_ctx);
            char    szName[64] = {};
            sprintf(szName, "child_%d", (int)i);
            child->setName(szName);
            rootNode->addChild(child);
        }
        _scene->addNodesToTree({rootNode});

        String          gltfFile    =   R"(E:\study\gltf\glTF-Sample-Assets\Models\BoxAnimated\glTF/BoxAnimated.gltf)";
        FEFileFormat    fmtText(".gltf","1.0.0.0","GLTF text Format!");

        auto            reader  =   FEFileFormatHelper::queryReader(_ctx,fmtText);
        if (reader)
        {
            auto    objects =   reader->readFiles({gltfFile});
            Nodes   nodes;
            for (auto var : objects)
            {   
                Node    node    =   var->cast<FENode>();
                if (node == nullptr)
                    continue;
                else
                    nodes.push_back(node);
            }
            _scene->dispatchNodesToSystem(nodes);
            _scene->addNodesToTree(nodes);
        }

        _timer = new QTimer(this);
        _timer->start(16); 
        connect(_timer, &QTimer::timeout, this, QOverload<>::of(&WigetViewer::update));
    }

    WigetViewer::~WigetViewer()
    {
    }

    void	WigetViewer::onEngineStart()
    {   
    }

    void    WigetViewer::paintEvent(QPaintEvent* )
    {
        if (_app)
        {
            _app->onMessage(MsgUpdate());
            _app->onMessage(MsgRender());
        }
    }
    void    WigetViewer::mousePressEvent(QMouseEvent* evt) 
    {
        if (_app == nullptr)
            return;
        FEMouseInfo info = {};

        info._prev      =   _prevMouse;
        info._mouse     =   int2(evt->pos().x(), evt->pos().y());
        _prevMouse      =   int2(evt->pos().x(), evt->pos().y());

        if (evt->modifiers() & Qt::ControlModifier)     info._states.addFlag(FEMouseInfo::CtrlPressed);
        if (evt->modifiers() & Qt::ShiftModifier)       info._states.addFlag(FEMouseInfo::ShiftPressed);
        if (evt->modifiers() & Qt::AltModifier)         info._states.addFlag(FEMouseInfo::AltPressed);

        switch(evt->button())
        {   
        case Qt::LeftButton:
            {
                info._states.addFlag(FEMouseInfo::LButtonPressed);
                _app->onMessage(MsgLButtonDown(info));
            }
            break;
        case Qt::RightButton:
            {
                info._states.addFlag(FEMouseInfo::RButtonPressed);
                _app->onMessage(MsgRButtonDown(info));
            }
            break;
        case Qt::MiddleButton:
            {
                info._states.addFlag(FEMouseInfo::MButtonPressed);
                _app->onMessage(MsgMButtonDown(info));
            }
            break;
        }
    }
    void    WigetViewer::mouseReleaseEvent(QMouseEvent *evt)
    {
        if (_app == nullptr)
            return;
        FEMouseInfo info = {};

        info._prev      =   _prevMouse;
        info._mouse     =   int2(evt->pos().x(), evt->pos().y());
        _prevMouse      =   int2(evt->pos().x(), evt->pos().y());

        if (evt->modifiers() & Qt::ControlModifier)     info._states.addFlag(FEMouseInfo::CtrlPressed);
        if (evt->modifiers() & Qt::ShiftModifier)       info._states.addFlag(FEMouseInfo::ShiftPressed);
        if (evt->modifiers() & Qt::AltModifier)         info._states.addFlag(FEMouseInfo::AltPressed);

        switch(evt->button())
        {   
        case Qt::LeftButton:
            {
                info._states.addFlag(FEMouseInfo::LButtonPressed);
                _app->onMessage(MsgLButtonUp(info));
            }
            break;
        case Qt::RightButton:
            {
                info._states.addFlag(FEMouseInfo::RButtonPressed);
                _app->onMessage(MsgRButtonUp(info));
            }
            break;
        case Qt::MiddleButton:
            {
                info._states.addFlag(FEMouseInfo::MButtonPressed);
                _app->onMessage(MsgMButtonUp(info));
            }
            break;
        }
    }
    void    WigetViewer::mouseDoubleClickEvent(QMouseEvent* evt) 
    {
        UNUSED(evt);
    }
    void    WigetViewer::mouseMoveEvent(QMouseEvent* evt)
    {
        if (_app == nullptr)
            return;
        FEMouseInfo info = {};
        info._mouse     =   int2(evt->pos().x(), evt->pos().y());
        info._prev      =   _prevMouse;  
        _prevMouse      =   info._mouse;

        if (evt->modifiers() & Qt::ControlModifier)     info._states.addFlag(FEMouseInfo::CtrlPressed);
        if (evt->modifiers() & Qt::ShiftModifier)       info._states.addFlag(FEMouseInfo::ShiftPressed);
        if (evt->modifiers() & Qt::AltModifier)         info._states.addFlag(FEMouseInfo::AltPressed);

        if (evt->buttons() & Qt::LeftButton)            info._states.addFlag(FEMouseInfo::LButtonPressed);
        if (evt->buttons() & Qt::RightButton)           info._states.addFlag(FEMouseInfo::RButtonPressed);
        if (evt->buttons() & Qt::MiddleButton)          info._states.addFlag(FEMouseInfo::MButtonPressed);

        _app->onMessage(MsgMouseMove(info));
        
        
    }
    void	WigetViewer::wheelEvent(QWheelEvent* evt)
    {
        if (_app)
        {
            auto    pos     =   evt->position().toPoint();
            auto    delta   =   evt->angleDelta().y();
            _app->onMessage(MsgMouseWheel({int2(pos.x(), pos.y()),delta}));
        }
    }
    void	WigetViewer::resizeEvent(QResizeEvent*  evt)
    {
        if (_app)
        {
            _app->onMessage(MsgResize({evt->size().width(), evt->size().height()}));
        }
    }
    void    WigetViewer::messageNotify(const FEMessage& msgIn)
    {
        if (_scene == nullptr)
            return;
        switch(msgIn.msgId())
        {
        case MSG_RESIZE         :
            _prepared   =   false;
            _scene->onMessage(msgIn);
            _prepared   =   true;
            return;
        case MSG_RESIZE_START   :
            break;
        case MSG_RESIZE_END     :
            break;
        case MSG_UPDATE         :   
            if (!_prepared)
                return;
            break;
        case MSG_RENDER         :
            if (!_prepared)
                return;
            break;
        }
        _scene->onMessage(msgIn);
    }

}



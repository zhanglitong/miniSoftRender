#include    "WigetViewer.h"
#include    "FEAppHelper.hpp"

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
    }

    WigetViewer::~WigetViewer()
    {
    }

    void	WigetViewer::onEngineStart()
    {   
    }

    void	WigetViewer::wheelEvent(QWheelEvent* evt)
    {
    }

    void	WigetViewer::resizeEvent(QResizeEvent*  evt)
    {
    }
    bool    WigetViewer::event(QEvent *event)
    {
        return QWidget::event(event);
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



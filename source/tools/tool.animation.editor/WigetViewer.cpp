#include    "WigetViewer.h"
#include    "FEAppHelper.hpp"
#include    "FEFileFormatHelper.hpp"

#if     FE_PLATFORM == FE_PLATFORM_WIN32
#include    <windows.h>
#endif

namespace   FE
{

    WigetViewer::WigetViewer(QWidget* parent)
        :QWidget(parent)
    {
        setAcceptDrops(true);
        setAttribute(Qt::WA_AcceptTouchEvents);
        /// 强制创建原生窗口,确保 winId() 返回有效的 HWND
        setAttribute(Qt::WA_NativeWindow);
        /// 创建，但没有初始化
        _scene      =   new FEScene(_ctx);
        /// 创建定时器,在 initEngine 完成后启动
        _timer = new QTimer(this);
        connect(_timer, &QTimer::timeout, this, QOverload<>::of(&WigetViewer::update));
    }

    WigetViewer::~WigetViewer()
    {
    }

    void    WigetViewer::initEngine()
    {
        if (_inited)
            return;
        /// 确保原生窗口已创建且尺寸有效
        auto    hwnd    =   (HWND)winId();
        if (hwnd == nullptr)
            return;
        RECT    rc;
        GetClientRect(hwnd, &rc);
        if (rc.right - rc.left <= 0 || rc.bottom - rc.top <= 0)
            return;

        FEApp::CreateInfo   info    =   {};
#if     FE_PLATFORM == FE_PLATFORM_WIN32
        info._appInst   =   GetModuleHandle(nullptr);
#endif
        info._window    =   (void*)hwnd;
        info._width     =   (uint)(rc.right  - rc.left);
        info._height    =   (uint)(rc.bottom - rc.top);
        info._notify    =   std::bind(&WigetViewer::messageNotify,this,std::placeholders::_1);
        _app    =   FE::FEAppHelper::create(_ctx,info);
        if (_app == nullptr)
            return;
        
        _scene->setup(_app);
        _inited     =   true;
        _timer->start(16);
    }

    void    WigetViewer::showEvent(QShowEvent* event)
    {
        if (!_inited)
            initEngine();
        QWidget::showEvent(event);
    }
    void    WigetViewer::closeEvent(QCloseEvent *event) 
    {
        if (_scene)
        {
            _scene->destroy();
        }
        QWidget::closeEvent(event);
    }

    void	WigetViewer::onEngineStart()
    {
    }

    void    WigetViewer::paintEvent(QPaintEvent* )
    {
        if (!_inited)
            initEngine();
        if (_app && _inited)
        {
            /// 如果尚未 prepared(构造时 setup 直接调用 FEScene::resize,
            /// 未经过 messageNotify 的 MSG_RESIZE 路径),用当前实际大小补触发一次
            if (!_prepared)
            {
                auto    r   =   rect();
                if (r.width() > 0 && r.height() > 0)
                    _app->onMessage(MsgResize({r.width(),r.height()}));
            }
            else
            {
                _app->onMessage(MsgUpdate());
                _app->onMessage(MsgRender());
            }
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
            return;
        case MSG_RESIZE_END     :
            return;
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



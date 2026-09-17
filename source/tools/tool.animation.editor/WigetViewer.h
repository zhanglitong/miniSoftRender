#pragma     once
#include    <QWidget>
#include    <QPushButton>
#include    <QFileDialog>
#include    <QDebug>
#include    <QIcon>
#include    <QPainter>
#include    <QSpacerItem>
#include    <QByteArray>
#include    <QPaintEvent>
#include    <QGridLayout>
#include    <QMessageBox>
#include    <QFileDialog>
#include    <QLabel>
#include    <QtWidgets/QMainWindow>
#include    <QMenu>
#include    <QTimer>
#include    <QCoreApplication>

#include    "FEContext.hpp"
#include    "graphic/FEScene.h"
#include    "FEDelegate.hpp"
using   namespace   FE;

namespace   FE
{
    /// <summary>
    /// 引擎创建完成后通知
    /// 有一些依赖引擎资源的模块可以在该通知里面完成
    /// </summary>
    using   NotifyEngineStart   =   FETMultiDelegate<void(FEScene&)>;

    class   WigetViewer :
        public QWidget
    {
        Q_OBJECT
    public:
        WigetViewer(QWidget* parent);
        ~WigetViewer();
    public:
        inline  Scene   scene() const
        {
            return  _scene;
        }
        /// <summary>
        /// 引擎初始化通知
        /// </summary>
        /// <returns></returns>
        inline  auto&   notify() 
        {
            return  _notify;
        }
    protected:
        virtual void    paintEvent(QPaintEvent *event)              override ;
        virtual void    mousePressEvent(QMouseEvent *event)         override ;
        virtual void    mouseReleaseEvent(QMouseEvent *event)       override ;
        virtual void    mouseDoubleClickEvent(QMouseEvent *event)   override ;
        virtual void    mouseMoveEvent(QMouseEvent *event)          override ;
        virtual void    wheelEvent(QWheelEvent *event)              override ;
        virtual void    resizeEvent(QResizeEvent *event)            override ;
        virtual void    showEvent(QShowEvent *event)                override;
        virtual void    closeEvent(QCloseEvent *event)              override ;
    public slots:
        /// <summary>
        /// 时间线通知
        /// </summary>
        void    slotTimeLineChanged(double time);
    protected:
        void	onEngineStart();
        void    messageNotify(const FEMessage& msgIn);
        void    initEngine();
    protected:
        App                 _app;
        Scene               _scene;
        NotifyEngineStart   _notify;
        QTimer*             _timer      =   nullptr;
        int2                _prevMouse  =   int2(0, 0);
        bool                _prepared   =   false;
        bool                _inited     =   false;
    };
}





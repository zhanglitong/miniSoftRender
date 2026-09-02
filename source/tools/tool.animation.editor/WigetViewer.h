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

#include    "FEContext.hpp"
#include    "graphic/FEScene.h"
using   namespace   FE;

namespace   FE
{
    class   WigetViewer :
        public QWidget
    {
        Q_OBJECT
    public:
        WigetViewer(QWidget* parent);
        ~WigetViewer();
    public:
        virtual void	onEngineStart();
        inline  Scene   scene() const
        {
            return  _scene;
        }
    protected:
        virtual void	wheelEvent(QWheelEvent* event) override;
        virtual void	resizeEvent(QResizeEvent *) override;
        virtual bool    event(QEvent *event) override;
    protected:
        void    messageNotify(const FEMessage& msgIn);
    protected:
        FEContext   _ctx;
        App         _app;
        Scene       _scene;
        bool        _prepared;
    };
}





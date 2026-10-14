#pragma     once

#include    <QWidget>
#include    <QLabel>
#include    "FEObject.h"

class PropsWidget : public QWidget
{
    Q_OBJECT
public:
    PropsWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        _label = new QLabel(this);
        _label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed));
    }
public:
    virtual void updateUi()
    {}

    virtual void setObeject(FE::FEObject* object)
    {}

    virtual void setTitle(const QString& text)
    {
        _label->setText(text);
    }
    /// <summary>
    /// 用于重置对象到出时状态
    /// 例如，释放页面引用的一些智能指针对象，避免引起资源泄露
    /// </summary>
    virtual void    reset()
    {}
protected:
    QLabel*             _label  =   nullptr;
    FE::FEObject*       _object =   nullptr;
};

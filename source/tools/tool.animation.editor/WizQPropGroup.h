#pragma     once
#include    "WizQProperty.h"
#include    <QPushButton>
#include    <QPainter>

class WizQPropGroup : public QWidget
{
public:
    Q_OBJECT
public:
    WizQPropGroup(const QString& title, QWidget* parent);
public:
    inline QWidget* propsWidget() const
    {
        return  _propsWidget;
    }
public:
    virtual void    paintEvent(QPaintEvent* evt) override;
public slots:
    void    slotExpandClicked();
private:
    std::vector<WizQProperty*>  _properties;

    QPushButton*    _titleButton;
    QVBoxLayout*    _layout;

    QWidget*        _propsWidget;

    QString         _title;
};

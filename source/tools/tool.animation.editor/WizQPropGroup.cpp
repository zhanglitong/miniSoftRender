#include    "WizQPropGroup.h"
#include    "MainWindow.h"

WizQPropGroup::WizQPropGroup(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    _title          =   title;
    _titleButton    =   new QPushButton(this);
    _titleButton->setText(_title);
    _titleButton->setStyleSheet("text-align: left;");
    _titleButton->setFlat(true);

    _titleButton->setIcon(OBJECT_ICON(ID_SUB));
    connect(_titleButton, &QPushButton::clicked, this, &WizQPropGroup::slotExpandClicked);

    _propsWidget    =   new QWidget(this);
    _propsWidget->setVisible(true);     /// 属性组默认展开

    auto    layout  =   new QVBoxLayout(this);
    layout->setContentsMargins({0, 0, 0, 0});
    layout->addWidget(_titleButton);
    layout->addWidget(_propsWidget);
}

void    WizQPropGroup::paintEvent(QPaintEvent* evt)
{
    QPainter    painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 220, 220, 200));
    painter.drawRoundedRect(rect(), 4, 4);
}

void    WizQPropGroup::slotExpandClicked()
{
    _propsWidget->setVisible(!_propsWidget->isVisible());
    _titleButton->setIcon(_propsWidget->isVisible() ? OBJECT_ICON(ID_SUB) : OBJECT_ICON(ID_PLUS));
}

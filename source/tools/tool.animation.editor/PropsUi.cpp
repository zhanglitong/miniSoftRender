#include    "PropsUi.h"
#include    "MainWindow.h"
#include    <QPainter>
#include    <QMouseEvent>

//////////////////////////////////////////////////////////////////////////////////
// SpinBoxEditHelper
//////////////////////////////////////////////////////////////////////////////////

void    SpinBoxEditHelper::getRects(int w, int h, QRect& subRect, QRect& midRect, QRect& addRect) const
{
    subRect =   QRect(0, 0, h, h);
    addRect =   QRect(w - h, 0, h, h);
    midRect =   QRect(h, 0, addRect.left() - subRect.right(), h);
}

void    SpinBoxEditHelper::paint(QPainter& painter, const QWidget* w, const QString& text) const
{
    QRect   subRect;
    QRect   addRect;
    QRect   midRect;
    getRects(w->width(), w->height(), subRect, midRect, addRect);

    QPen    pen;
    pen.setColor(Qt::white);
    if (_isCursorIn)
    {
        painter.setPen(Qt::NoPen);
        /// 绘制subRect
        painter.setBrush(_subHover ? _hoverColor : _bgColor);
        painter.drawRect(subRect);
        painter.drawPixmap(subRect, _mainApp->objectIcon(ID_SUB).pixmap(subRect.width(), subRect.height()));

        /// 绘制midRect
        painter.setBrush(_midHover ? _hoverColor : _bgColor);
        painter.drawRect(midRect);

        /// 绘制addRect
        painter.setBrush(_addHover ? _hoverColor : _bgColor);
        painter.drawRect(addRect);
        painter.drawPixmap(addRect, _mainApp->objectIcon(ID_PLUS).pixmap(addRect.width(), addRect.height()));

        /// 绘制文本
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.drawText(midRect, Qt::AlignCenter, text);
    }
    else
    {
        /// 绘制文本
        pen.setColor(Qt::black);
        painter.setPen(pen);
        painter.fillRect(w->rect(), _bgColor);
        painter.drawText(midRect, Qt::AlignCenter, text);
    }
}

bool    SpinBoxEditHelper::updateHover(const QPoint& pos, int w, int h)
{
    QRect   subRect;
    QRect   addRect;
    QRect   midRect;
    getRects(w, h, subRect, midRect, addRect);
    if (subRect.contains(pos))
    {
        _subHover   =   true;
        _midHover   =   false;
        _addHover   =   false;
        return  true;
    }
    else if (addRect.contains(pos))
    {
        _subHover   =   false;
        _midHover   =   false;
        _addHover   =   true;
        return  true;
    }
    else if (midRect.contains(pos))
    {
        _subHover   =   false;
        _midHover   =   true;
        _addHover   =   false;
        return  true;
    }
    return  false;
}

void    SpinBoxEditHelper::resetHover()
{
    _subHover   =   false;
    _midHover   =   false;
    _addHover   =   false;
    _isCursorIn =   false;
}

QPoint  SpinBoxEditHelper::clampDragPos(const QPoint& curPos, const QWidget* w)
{
    if (w->rect().contains(curPos, true))
        return  curPos;

    QPoint  newPos  =   curPos;
    if (curPos.y() < 0)
    {
        newPos.setY(0);
    }
    else if (curPos.y() > w->height())
    {
        newPos.setY(w->height());
    }

    auto    curX    =   curPos.x();
    if (curX < 0)
    {
        newPos.setX(w->width());
        _leftDownPos.setX(_leftDownPos.x() + w->width());
    }
    else if (curX > w->width())
    {
        newPos.setX(0);
        _leftDownPos.setX(_leftDownPos.x() - w->width());
    }
    QCursor::setPos(w->mapToGlobal(newPos));
    return  newPos;
}

//////////////////////////////////////////////////////////////////////////////////
// DoubleEditingSpinBox
//////////////////////////////////////////////////////////////////////////////////

DoubleEditingSpinBox::DoubleEditingSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
    connect(this,   SIGNAL(editingFinished()), this, SLOT(slotEditFinished()));
    _beginValue     =   value();
    setMouseTracking(true);
    this->setFocusPolicy(Qt::TabFocus);
    this->lineEdit()->setVisible(false);
    this->setButtonSymbols(ButtonSymbols::NoButtons);
}

void    DoubleEditingSpinBox::slotEditFinished()
{
    this->blockSignals(true);
    this->clearFocus();
    this->blockSignals(false);
    emit    sigEditFinished(_beginValue, value());
}

void    DoubleEditingSpinBox::focusInEvent(QFocusEvent* evt)
{
    _beginValue =   value();
    QDoubleSpinBox::focusInEvent(evt);
    lineEdit()->setVisible(true);
    lineEdit()->selectAll();
}

void    DoubleEditingSpinBox::focusOutEvent(QFocusEvent* evt)
{
    QDoubleSpinBox::focusOutEvent(evt);
    lineEdit()->setVisible(false);
}

void    DoubleEditingSpinBox::paintEvent(QPaintEvent* evt)
{
    if (hasFocus())
    {
        QDoubleSpinBox::paintEvent(evt);
        return;
    }
    QPainter    painter(this);
    _helper.paint(painter, this, lineEdit()->text());
}

void    DoubleEditingSpinBox::leaveEvent(QEvent* event)
{
    _helper.resetHover();
    update();
    setCursor(Qt::ArrowCursor);
    QDoubleSpinBox::leaveEvent(event);
}

void    DoubleEditingSpinBox::enterEvent(QEnterEvent* event)
{
    setCursor(Qt::SizeHorCursor);
    QDoubleSpinBox::enterEvent(event);
    _helper._isCursorIn =   true;
}

void    DoubleEditingSpinBox::mouseMoveEvent(QMouseEvent* event)
{
    if (hasFocus())
        return;

    if (_helper._leftDown)
    {
        if (!_helper._dragging)
        {
            _helper._dragging   =   true;
            _beginValue         =   value();
        }
        auto    curPos  =   _helper.clampDragPos(event->pos(), this);
        auto    deltaX  =   curPos.x() - _helper._leftDownPos.x();

        setValue(_beginValue + (double)deltaX * singleStep());
    }
    else
    {
        if (_helper.updateHover(event->pos(), width(), height()))
            update();
    }
}

void    DoubleEditingSpinBox::mousePressEvent(QMouseEvent* event)
{
    if (hasFocus())
    {
        QDoubleSpinBox::mousePressEvent(event);
        return;
    }
    _helper._leftDown       =   true;
    _helper._leftDownPos    =   event->pos();
}

void    DoubleEditingSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (hasFocus())
    {
        QDoubleSpinBox::mouseReleaseEvent(event);
        return;
    }
    _helper._leftDown   =   false;
    auto    pos =   event->pos();

    if (_helper._dragging)
    {
        emit    sigEditFinished(_beginValue, value());
        _helper._dragging   =   false;
    }
    else if (pos == _helper._leftDownPos)
    {
        QRect   subRect;
        QRect   addRect;
        QRect   midRect;
        _helper.getRects(width(), height(), subRect, midRect, addRect);
        if (subRect.contains(pos))
        {
            _beginValue =   value();
            setValue(value() - singleStep());
            emit    sigEditFinished(_beginValue, value());
        }
        else if (addRect.contains(pos))
        {
            _beginValue =   value();
            setValue(value() + singleStep());
            emit    sigEditFinished(_beginValue, value());
        }
        else if (midRect.contains(pos))
        {
            setFocus();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
// IntEditingSpinBox
//////////////////////////////////////////////////////////////////////////////////

IntEditingSpinBox::IntEditingSpinBox(QWidget* parent)
    : QSpinBox(parent)
{
    connect(this,   SIGNAL(editingFinished()), this, SLOT(slotEditFinished()));
    _beginValue =   value();
    setMouseTracking(true);
    this->setFocusPolicy(Qt::TabFocus);
    this->lineEdit()->setVisible(false);
    this->setButtonSymbols(ButtonSymbols::NoButtons);
}

void    IntEditingSpinBox::slotEditFinished()
{
    this->blockSignals(true);
    this->clearFocus();
    this->blockSignals(false);
    emit    sigEditFinished(_beginValue, value());
}

void    IntEditingSpinBox::focusInEvent(QFocusEvent* evt)
{
    _beginValue =   value();
    QSpinBox::focusInEvent(evt);
    lineEdit()->setVisible(true);
    lineEdit()->selectAll();
}

void    IntEditingSpinBox::focusOutEvent(QFocusEvent* evt)
{
    QSpinBox::focusOutEvent(evt);
    lineEdit()->setVisible(false);
}

void    IntEditingSpinBox::paintEvent(QPaintEvent* evt)
{
    if (hasFocus())
    {
        QSpinBox::paintEvent(evt);
        return;
    }
    QPainter    painter(this);
    _helper.paint(painter, this, lineEdit()->text());
}

void    IntEditingSpinBox::leaveEvent(QEvent* event)
{
    _helper.resetHover();
    update();
    setCursor(Qt::ArrowCursor);
    QSpinBox::leaveEvent(event);
}

void    IntEditingSpinBox::enterEvent(QEnterEvent* event)
{
    setCursor(Qt::SizeHorCursor);
    QSpinBox::enterEvent(event);
    _helper._isCursorIn =   true;
}

void    IntEditingSpinBox::mouseMoveEvent(QMouseEvent* event)
{
    if (hasFocus())
        return;

    if (_helper._leftDown)
    {
        if (!_helper._dragging)
        {
            _helper._dragging   =   true;
            _beginValue         =   value();
        }
        auto    curPos  =   _helper.clampDragPos(event->pos(), this);
        auto    deltaX  =   curPos.x() - _helper._leftDownPos.x();

        setValue(_beginValue + deltaX * singleStep());
    }
    else
    {
        if (_helper.updateHover(event->pos(), width(), height()))
            update();
    }
}

void    IntEditingSpinBox::mousePressEvent(QMouseEvent* event)
{
    if (hasFocus())
    {
        QSpinBox::mousePressEvent(event);
        return;
    }
    _helper._leftDown       =   true;
    _helper._leftDownPos    =   event->pos();
}

void    IntEditingSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
    if (hasFocus())
    {
        QSpinBox::mouseReleaseEvent(event);
        return;
    }
    _helper._leftDown   =   false;
    auto    pos =   event->pos();

    if (_helper._dragging)
    {
        emit    sigEditFinished(_beginValue, value());
        _helper._dragging   =   false;
    }
    else if (pos == _helper._leftDownPos)
    {
        QRect   subRect;
        QRect   addRect;
        QRect   midRect;
        _helper.getRects(width(), height(), subRect, midRect, addRect);
        if (subRect.contains(pos))
        {
            _beginValue =   value();
            setValue(value() - singleStep());
            emit    sigEditFinished(_beginValue, value());
        }
        else if (addRect.contains(pos))
        {
            _beginValue =   value();
            setValue(value() + singleStep());
            emit    sigEditFinished(_beginValue, value());
        }
        else if (midRect.contains(pos))
        {
            setFocus();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
// TrackCheckBox
//////////////////////////////////////////////////////////////////////////////////

TrackCheckBox::TrackCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSizePolicy(sizePolicy);
}
